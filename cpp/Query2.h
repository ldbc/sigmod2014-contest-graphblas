#pragma once

#include <queue>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <memory>
#include <set>
#include <cstdio>
#include <utility>
#include "utils.h"
#include "Query.h"

class Query2 : public Query<int, std::string> {
    int top_k_limit;
    std::string const &birthday_limit_str;

    std::tuple<std::string, std::string> initial_calculation() override {
        // make sure time_t is converted correctly
        GrB_Type GB_TIME_T = GrB_INT64;
        static_assert(std::is_same<time_t, int64_t>::value);

        // store scalar parameter
        GBxx_Object<GxB_Scalar> birthday_limit = GB(GxB_Scalar_new, GB_TIME_T);
        ok(GxB_Scalar_setElement_INT64(birthday_limit.get(), parseTimestamp(birthday_limit_str.c_str(), DateFormat)));

        // mask of persons based on their birthdays
        GBxx_Object<GrB_Vector> birthday_person_mask = GB(GrB_Vector_new, GB_TIME_T, input.persons.size());
        std::vector<GrB_Index> all_indices;
        std::vector<time_t> birthdays;
        birthdays.resize(input.persons.size());
        all_indices.resize(input.persons.size());
        std::transform(input.persons.vertices.begin(), input.persons.vertices.end(), birthdays.begin(),
                       [](auto &p) { return p.birthday; });
        std::iota(all_indices.begin(), all_indices.end(), 0);
        ok(GrB_Vector_build_INT64(birthday_person_mask.get(), all_indices.data(), birthdays.data(), birthdays.size(),
                                  GrB_PLUS_INT64));

        ok(GxB_Vector_select(birthday_person_mask.get(), GrB_NULL, GrB_NULL,
                             GxB_GE_THUNK, birthday_person_mask.get(),
                             birthday_limit.get(), GrB_NULL));

        std::vector<std::tuple<uint64_t, std::reference_wrapper<std::string const>>> tag_scores;
        tag_scores.reserve(input.tags.size());
        GBxx_Object<GrB_Vector> interested_person_vec = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());
        for (int tag_index = 0; tag_index < input.tags.size(); ++tag_index) {
            ok(GrB_Col_extract(interested_person_vec.get(), birthday_person_mask.get(), GrB_NULL,
                               input.hasInterestTran.matrix.get(), GrB_ALL, 0, tag_index,
                               GrB_DESC_RST0));

            GrB_Index interested_person_nvals;
            ok(GrB_Vector_nvals(&interested_person_nvals, interested_person_vec.get()));

            uint64_t score = 0;
            if (interested_person_nvals != 0) {
                std::vector<GrB_Index> interested_person_indices(interested_person_nvals);
                GrB_Index nvals_out = interested_person_nvals;
                ok(GrB_Vector_extractTuples_BOOL(interested_person_indices.data(), nullptr, &nvals_out,
                                                 interested_person_vec.get()));
                assert(interested_person_nvals == nvals_out);

                GBxx_Object<GrB_Matrix> knows_subgraph = GB(GrB_Matrix_new, GrB_BOOL, interested_person_nvals,
                                                            interested_person_nvals);

                ok(GrB_Matrix_extract(knows_subgraph.get(), GrB_NULL, GrB_NULL, input.knows.matrix.get(),
                                      interested_person_indices.data(), interested_person_nvals,
                                      interested_person_indices.data(), interested_person_nvals,
                                      GrB_NULL));

                // assuming that all component_ids will be in [0, n)
                GBxx_Object<GrB_Vector> components_vector = GB(LAGraph_cc_fastsv, knows_subgraph.get(), false);

                std::vector<uint64_t> components(interested_person_nvals),
                        component_sizes(interested_person_nvals);

                // GrB_NULL to avoid extracting matrix values (SuiteSparse extension)
                nvals_out = interested_person_nvals;
                ok(GrB_Vector_extractTuples_UINT64(GrB_NULL, components.data(), &nvals_out, components_vector.get()));
                assert(interested_person_nvals == nvals_out);

                // count size of each component
                for (auto component_id:components)
                    ++component_sizes[component_id];

                score = *std::max_element(component_sizes.begin(), component_sizes.end());
            }
            tag_scores.emplace_back(score, input.tags.vertices[tag_index].name);
        }

        std::sort(tag_scores.begin(), tag_scores.end(), transformComparator([](const auto &val) {
            return std::make_tuple(
                    std::numeric_limits<uint64_t>::max() - std::get<0>(val),
                    std::get<1>(val));
        }));
        tag_scores.erase(tag_scores.begin() + top_k_limit, tag_scores.end());

        std::string result, comment;
        for (int i = 0; i < tag_scores.size(); ++i) {
            auto const &pair = tag_scores[i];

            if (i != 0) {
                result += ' ';
                comment += ' ';
            }

            result += std::get<1>(pair).get();
            comment += std::to_string(std::get<0>(pair));
        }

        return {result, comment};
    }

public:
    Query2(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input),
              top_k_limit(std::get<0>(queryParams)), birthday_limit_str(std::get<1>(queryParams)) {}
};
