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
#include <cstdio>
#include <string>
#include <iostream>

class Query1 : public Query<uint64_t, uint64_t, int> {
    GrB_Index p1, p2;
    int comment_lower_limit;

    std::tuple<std::string, std::string> initial_calculation() override {

        GrB_Matrix filtered_matrix_ptr;

        GBxx_Object<GrB_Matrix> personAToComment2 = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                       input.comments.size());

        ok(GrB_mxm(personAToComment2.get(), GrB_NULL, GrB_NULL, GxB_PLUS_TIMES_INT64, input.hasCreatorTran.matrix.get(),
                   input.replyOf.matrix.get(), GrB_NULL));
        // ok(GxB_Matrix_fprint(personAToComment2.get(), "personAToComment2", GxB_SUMMARY, stdout));

        GBxx_Object<GrB_Matrix> personToPerson = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                    input.persons.size());

        ok(GrB_mxm(personToPerson.get(), input.knows.matrix.get(), GrB_NULL, GxB_PLUS_TIMES_INT64,
                   personAToComment2.get(), input.hasCreator.matrix.get(), GrB_NULL));
        // ok(GxB_Matrix_fprint(personToPerson.get(), "personToPerson", GxB_SUMMARY, stdout));

        if (comment_lower_limit == -1) {
            filtered_matrix_ptr = input.knows.matrix.get();

        } else {
            auto limit = GB(GxB_Scalar_new, GrB_INT32);
            ok(GxB_Scalar_setElement_INT32(limit.get(), comment_lower_limit));
            ok(GxB_Matrix_select(personToPerson.get(), GrB_NULL, GrB_NULL, GxB_GT_THUNK, personToPerson.get(),
                                 limit.get(), GrB_NULL));
            // make symmetric by removing one-directional freqComm relationships
            ok(GrB_eWiseMult_Matrix_BinaryOp(personToPerson.get(), GrB_NULL, GrB_NULL,
                                             GxB_PAIR_UINT64, personToPerson.get(), personToPerson.get(), GrB_DESC_T0));

            filtered_matrix_ptr = personToPerson.get();
        }

#ifndef NDEBUG
        ok(GxB_Matrix_fprint(filtered_matrix_ptr, "personToPersonFiltered", GxB_SUMMARY, stdout));
#endif

        //Person to person is symmtetric, so no need to transpose
        GBxx_Object<GrB_Vector> v_output = GB(LAGraph_bfs_pushpull, nullptr, filtered_matrix_ptr, filtered_matrix_ptr,
                                              p1, GrB_NULL, false);
#ifndef NDEBUG
        ok(GxB_Vector_fprint(v_output.get(), "output_vec", GxB_SUMMARY, stdout));
#endif

        int result;
        ok(GrB_Vector_extractElement_INT32(&result, v_output.get(), p2));
        //Decrementing result, because levels in BFS start at 1
        result--;
        std::string result_str, comment_str;

        result_str = std::to_string(result);
        return {result_str, comment_str};
    }

public:
    Query1(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        uint64_t p1_id, p2_id;

        std::tie(p1_id, p2_id, comment_lower_limit) = queryParams;

        ok(GrB_Vector_extractElement_UINT64(&p1, input.persons.idToIndex.get(), p1_id));
        ok(GrB_Vector_extractElement_UINT64(&p2, input.persons.idToIndex.get(), p2_id));
    }
};
