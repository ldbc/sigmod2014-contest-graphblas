#pragma once

#include "ccv.h"
#include "Query.h"
#include "utils.h"

#include <queue>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <memory>
#include <set>
#include <cstdio>
#include <utility>
#include <cstdio>

class Query4 : public Query<int, std::string> {
    int topKLimit;
    std::string tagName;

    std::tuple<std::string, std::string> initial_calculation() override {
        // find tag
        GrB_Index tag_index = input.tags.findIndexByName(tagName);

        // hasTag
        GBxx_Object<GrB_Vector> relevant_forums = GB(GrB_Vector_new, GrB_BOOL, input.forums.size());
        ok(GrB_Col_extract(relevant_forums.get(), GrB_NULL, GrB_NULL,
                           input.hasTag.matrix.get(), GrB_ALL, 0,
                           tag_index, GrB_NULL));
        // hasMember
        GBxx_Object<GrB_Vector> relevant_persons = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());
        ok(GrB_vxm(relevant_persons.get(), GrB_NULL, GrB_NULL,
                   GxB_LOR_LAND_BOOL, relevant_forums.get(), input.hasMember.matrix.get(), GrB_NULL));

        // transform relevant_persons vec. to array
        GrB_Index relevant_persons_nvals;
        ok(GrB_Vector_nvals(&relevant_persons_nvals, relevant_persons.get()));
        std::vector<GrB_Index> relevant_person_indices(relevant_persons_nvals);
        {
            GrB_Index nvals_out = relevant_persons_nvals;
            ok(GrB_Vector_extractTuples_BOOL(relevant_person_indices.data(), nullptr, &nvals_out,
                                             relevant_persons.get()));
            assert(relevant_persons_nvals == nvals_out);
        }

        // extract member_friends submatrix
        GBxx_Object<GrB_Matrix> member_friends = GB(GrB_Matrix_new, GrB_BOOL,
                                                    relevant_persons_nvals, relevant_persons_nvals);
        ok(GrB_Matrix_extract(member_friends.get(), GrB_NULL, GrB_NULL, input.knows.matrix.get(),
                              relevant_person_indices.data(), relevant_person_indices.size(),
                              relevant_person_indices.data(), relevant_person_indices.size(), GrB_NULL));

//        // diagonal sources matrix
//        GBxx_Object<GrB_Matrix> sources = GB(GrB_Matrix_new, GrB_BOOL,
//                                             relevant_persons_nvals, relevant_persons_nvals);
//        std::vector<GrB_Index> all_indices;
//        all_indices.resize(relevant_persons_nvals);
//        std::iota(all_indices.begin(), all_indices.end(), 0);
//        ok(GrB_Matrix_build_BOOL(sources.get(),
//                                 all_indices.data(), all_indices.data(), array_of_true(relevant_persons_nvals).get(),
//                                 relevant_persons_nvals, GrB_SECOND_BOOL));

        // call MSBFS-based closeness centrality value computation
        GrB_Vector ccv_ptr = nullptr;
        ok(compute_ccv(&ccv_ptr, member_friends.get()));
        GBxx_Object<GrB_Vector> ccv{ccv_ptr};

        // extract tuples from ccv result
        GrB_Index ccv_nvals;
        ok(GrB_Vector_nvals(&ccv_nvals, ccv.get()));
        std::vector<GrB_Index> cc_indices(ccv_nvals);
        std::vector<double> cc_values(ccv_nvals);
        {
            GrB_Index nvals_out = relevant_persons_nvals;
            ok(GrB_Vector_extractTuples_FP64(cc_indices.data(), cc_values.data(), &nvals_out, ccv.get()));
//            assert(relevant_persons_nvals == nvals_out); // TODO: what does happen if a person doesn't have CCV?
        }

        // define comparator for top scores
        using person_score_type = std::tuple<double, uint64_t>;
        // use a comparator which transforms the value for comparison
        auto comparator = transformComparator([](const auto &val) {
            return std::make_tuple(
                    -std::get<0>(val),  // score DESC
                    std::get<1>(val));  // person_id ASC
        });
        auto person_scores = makeSmallestElementsContainer<person_score_type>(topKLimit, comparator);

        // collect top scores
        for (size_t i = 0; i < cc_values.size(); ++i) {
            double score = cc_values[i];
            
            GrB_Index person_local_index = cc_indices[i];
            GrB_Index person_index = relevant_person_indices[person_local_index];
            uint64_t person_id = input.persons.vertexIds[person_index];
            
            person_scores.add({score, person_id});
        }

        std::string result, comment;
        bool firstIter = true;
        for (auto[score, person_id]: person_scores.removeElements()) {
            if (firstIter)
                firstIter = false;
            else {
                result += ' ';
                comment += ' ';
            }

            result += std::to_string(person_id);
            comment += std::to_string(score);
        }

        return {result, comment};
    }

public:
    Query4(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        std::tie(topKLimit, tagName) = queryParams;
    }
};
