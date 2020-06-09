#pragma once

#include "utils.h"
#include "Query.h"
#include "SmallestElementsContainer.h"

#include <queue>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <memory>
#include <set>
#include <cstdio>
#include <utility>

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
        ok(GrB_Vector_build_INT64(birthday_person_mask.get(),
                                  array_of_indices(input.persons.size()).get(),
                                  input.persons.birthdays.data(),
                                  input.persons.birthdays.size(),
                                  GrB_PLUS_INT64));

        ok(GxB_Vector_select(birthday_person_mask.get(), GrB_NULL, GrB_NULL,
                             GxB_GE_THUNK, birthday_person_mask.get(),
                             birthday_limit.get(), GrB_NULL));

        // store the score and a reference to the tag name
        using tag_score_type = std::tuple<uint64_t, std::reference_wrapper<std::string const>>;
        // use a comparator which transforms the value for comparison
        auto comparator = transformComparator([](const auto &val) {
            return std::make_tuple(
                    // invert order of unsigned integer
                    std::numeric_limits<uint64_t>::max() - std::get<0>(val), // score DESC
                    std::get<1>(val));                                       // tag_name ASC
        });

        auto tag_scores = makeSmallestElementsContainer<tag_score_type>(top_k_limit, comparator);

#pragma omp parallel num_threads(GlobalNThreads)
        {
            auto tag_scores_local = makeSmallestElementsContainer<tag_score_type>(top_k_limit, comparator);
            GBxx_Object<GrB_Vector> interested_person_vec = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());

#pragma omp for schedule(dynamic)
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
                    ok(GrB_Vector_extractTuples_UINT64(GrB_NULL, components.data(), &nvals_out,
                                                       components_vector.get()));
                    assert(interested_person_nvals == nvals_out);

                    // count size of each component
                    for (auto component_id:components)
                        ++component_sizes[component_id];

                    score = *std::max_element(component_sizes.begin(), component_sizes.end());
                }
                tag_scores_local.add({score, input.tags.names[tag_index]});
            }

#pragma omp critical(Q2_merge_thread_local_toplists)
            for (auto score : tag_scores_local.removeElements()) {
                tag_scores.add(score);
            }
        }

        std::string result, comment;
        bool firstIter = true;
        for (auto const &[score, tag_name]: tag_scores.removeElements()) {
            if (firstIter)
                firstIter = false;
            else {
                result += ' ';
                comment += ' ';
            }

            result += tag_name;
            comment += std::to_string(score);
        }

        return {result, comment};
    }

public:
    Query2(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input),
              top_k_limit(std::get<0>(queryParams)), birthday_limit_str(std::get<1>(queryParams)) {}
};
