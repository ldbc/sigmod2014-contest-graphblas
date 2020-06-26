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
        if (p1 == p2) {
            return {"0", ""};
        }

        GrB_Matrix A;
        GBxx_Object<GrB_Matrix> personToPerson;

        if (comment_lower_limit == -1) {
            A = input.knows.matrix.get();
        } else {
            GBxx_Object<GrB_Matrix> personAToComment2 = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                           input.comments.size());

            ok(GrB_mxm(personAToComment2.get(), GrB_NULL, GrB_NULL, GxB_PLUS_TIMES_INT64, input.hasCreator.matrix.get(),
                       input.replyOf.matrix.get(), GrB_DESC_T0));
            // ok(GxB_Matrix_fprint(personAToComment2.get(), "personAToComment2", GxB_SUMMARY, stdout));

            personToPerson = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                        input.persons.size());

            ok(GrB_mxm(personToPerson.get(), input.knows.matrix.get(), GrB_NULL, GxB_PLUS_TIMES_INT64,
                       personAToComment2.get(), input.hasCreator.matrix.get(), GrB_DESC_S));
            // ok(GxB_Matrix_fprint(personToPerson.get(), "personToPerson", GxB_SUMMARY, stdout));

            auto limit = GB(GxB_Scalar_new, GrB_INT32);
            ok(GxB_Scalar_setElement_INT32(limit.get(), comment_lower_limit));
            ok(GxB_Matrix_select(personToPerson.get(), GrB_NULL, GrB_NULL, GxB_GT_THUNK, personToPerson.get(),
                                 limit.get(), GrB_NULL));
            // make symmetric
            ok(GrB_eWiseMult_Matrix_BinaryOp(personToPerson.get(), GrB_NULL, GrB_NULL,
                                             GxB_PAIR_UINT64, personToPerson.get(), personToPerson.get(), GrB_DESC_T0));

            A = personToPerson.get();
        }

#ifndef NDEBUG
        ok(GxB_Matrix_fprint(A, "personToPersonFiltered", GxB_SUMMARY, stdout));
#endif

        int distance;

        GrB_Index n;
        ok(GrB_Matrix_nrows(&n, A));

        GBxx_Object<GrB_Vector> next1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> next2 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> intersection1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> intersection2 = GB(GrB_Vector_new, GrB_BOOL, n);

        ok(GrB_Vector_setElement_BOOL(next1.get(), true, p1));
        ok(GrB_Vector_setElement_BOOL(next2.get(), true, p2));
        GBxx_Object<GrB_Vector> seen1 = GB(GrB_Vector_dup, next1.get());
        GBxx_Object<GrB_Vector> seen2 = GB(GrB_Vector_dup, next2.get());

        // use two "push" frontiers
        for (GrB_Index level = 1; level < n / 2 + 1; level++) {
            ok(GrB_vxm(next1.get(), seen1.get(), NULL, GxB_ANY_PAIR_BOOL, next1.get(), A, GrB_DESC_RC));

            GrB_Index next1nvals;
            ok(GrB_Vector_nvals(&next1nvals, next1.get()));
            if (next1nvals == 0) {
                distance = -1;
                break;
            }

            ok(GrB_Vector_eWiseMult_BinaryOp(intersection1.get(), NULL, NULL, GrB_LAND, next1.get(), next2.get() /*prev next2*/, NULL));

            GrB_Index intersection1_nvals;
            ok(GrB_Vector_nvals(&intersection1_nvals, intersection1.get()));
            if (intersection1_nvals > 0) {
                distance = level * 2 - 1;
                break;
            }

            ok(GrB_vxm(next2.get(), seen2.get(), NULL, GxB_ANY_PAIR_BOOL, next2.get(), A, GrB_DESC_RC));
            ok(GrB_Vector_eWiseMult_BinaryOp(intersection2.get(), NULL, NULL, GrB_LAND, next1.get(), next2.get() /*current next2*/, NULL));

            GrB_Index intersection2_nvals;
            ok(GrB_Vector_nvals(&intersection2_nvals, intersection2.get()));
            if (intersection2_nvals > 0) {
                distance = level * 2;
                break;
            }

            GrB_Index next2nvals;
            ok(GrB_Vector_nvals(&next2nvals, next2.get()));
            if (next2nvals == 0) {
                distance = -1;
                break;
            }

            ok(GrB_eWiseAdd_Vector_BinaryOp(seen1.get(), NULL, NULL, GrB_LOR, seen1.get(), next1.get(), NULL));
            ok(GrB_eWiseAdd_Vector_BinaryOp(seen2.get(), NULL, NULL, GrB_LOR, seen2.get(), next2.get(), NULL));
        }

#ifndef NDEBUG
//        ok(GxB_Vector_fprint(v_output.get(), "output_vec", GxB_SUMMARY, stdout));
#endif

        std::string result_str, comment_str;
        result_str = std::to_string(distance);
        return {result_str, comment_str};
    }

public:
    Query1(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        uint64_t p1_id, p2_id;

        std::tie(p1_id, p2_id, comment_lower_limit) = queryParams;

        p1 = input.persons.idToIndex(p1_id);
        p2 = input.persons.idToIndex(p2_id);
    }
};
