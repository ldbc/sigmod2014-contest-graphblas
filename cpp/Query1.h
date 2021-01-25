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

    std::tuple<GrB_Index, GrB_Index> componentSizes(GrB_Matrix A, GrB_Index n) {
        // assuming that all component_ids will be in [0, n)
        GBxx_Object<GrB_Matrix> A_dup = GB(GrB_Matrix_dup, A);
        GrB_Matrix A_owning_ptr = A_dup.release();
        GBxx_Object<GrB_Vector> components_vector = GB(LAGraph_cc_fastsv5b, &A_owning_ptr, false);
        A_dup.reset(A_owning_ptr);

        std::vector<uint64_t> components(n);

        // GrB_NULL to avoid extracting matrix values (SuiteSparse extension)
        GrB_Index nvals_out = n;
        ok(GrB_Vector_extractTuples_UINT64(GrB_NULL, components.data(), &nvals_out, components_vector.get()));
        assert(n == nvals_out);

        return {
                std::count(components.begin(), components.end(), p1),
                std::count(components.begin(), components.end(), p2)
        };
    }

    std::tuple<std::string, std::string> initial_calculation() override {
        if (p1 == p2) {
            return {"0", ""};
        }

        GrB_Matrix A;
        // lifecycle prevents moving this into else branch
        GBxx_Object<GrB_Matrix> personToPerson;

        if (comment_lower_limit == -1) {
            A = input.knows.matrix.get();
        } else {
            GBxx_Object<GrB_Matrix> personAToComment2 = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                           input.comments.size());

            ok(GrB_mxm(personAToComment2.get(), GrB_NULL, GrB_NULL, GxB_PLUS_TIMES_INT64, input.hasCreator.matrix.get(),
                       input.replyOf.matrix.get(), GrB_DESC_T0));
            // ok(GxB_Matrix_fprint(personAToComment2.get(), "personAToComment2", GxB_SUMMARY, stdout));
            add_comment_if_on("personAToComment2", std::to_string(GBxx_nvals(personAToComment2)));

            personToPerson = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                input.persons.size());

            ok(GrB_mxm(personToPerson.get(), input.knows.matrix.get(), GrB_NULL, GxB_PLUS_TIMES_INT64,
                       personAToComment2.get(), input.hasCreator.matrix.get(), GrB_DESC_S));
            // ok(GxB_Matrix_fprint(personToPerson.get(), "personToPerson", GxB_SUMMARY, stdout));
            add_comment_if_on("personToPerson", std::to_string(GBxx_nvals(personToPerson)));

            auto limit = GB(GxB_Scalar_new, GrB_INT32);
            ok(GxB_Scalar_setElement_INT32(limit.get(), comment_lower_limit));
            ok(GxB_Matrix_select(personToPerson.get(), GrB_NULL, GrB_NULL, GxB_GT_THUNK, personToPerson.get(),
                                 limit.get(), GrB_NULL));
            // make symmetric
            ok(GrB_Matrix_eWiseMult_BinaryOp(personToPerson.get(), GrB_NULL, GrB_NULL,
                                             GxB_PAIR_UINT64, personToPerson.get(), personToPerson.get(), GrB_DESC_T0));

            A = personToPerson.get();
        }

#ifndef NDEBUG
        ok(GxB_Matrix_fprint(A, "personToPersonFiltered", GxB_SUMMARY, stdout));
#endif
        add_comment_if_on("A", std::to_string(GBxx_nvals(A)));

        int distance = 0;

        GrB_Index n;
        ok(GrB_Matrix_nrows(&n, A));

        auto[p1ComponentSize, p2ComponentSize] = componentSizes(A, n);
        add_comment_if_on("p1ComponentSize", std::to_string(p1ComponentSize));
        add_comment_if_on("p2ComponentSize", std::to_string(p2ComponentSize));

        GBxx_Object<GrB_Vector> next1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> next2 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> intersection1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> intersection2 = GB(GrB_Vector_new, GrB_BOOL, n);

        ok(GrB_Vector_setElement_BOOL(next1.get(), true, p1));
        ok(GrB_Vector_setElement_BOOL(next2.get(), true, p2));
        GBxx_Object<GrB_Vector> seen1 = GB(GrB_Vector_dup, next1.get());
        GBxx_Object<GrB_Vector> seen2 = GB(GrB_Vector_dup, next2.get());

        // use two "push" frontiers
        GrB_Index level;
#ifdef PRINT_EXTRA_COMMENTS
        GrB_Index next1_max = 0, next2_max = 0;
#endif
        for (level = 1; level < n / 2 + 1; level++) {
            ok(GrB_vxm(next1.get(), seen1.get(), NULL, GxB_ANY_PAIR_BOOL, next1.get(), A, GrB_DESC_RSC));

            GrB_Index next1nvals = GBxx_nvals(next1);
#ifdef PRINT_EXTRA_COMMENTS
            next1_max = std::max(next1_max, next1nvals);
#endif
            if (next1nvals == 0) {
                distance = -1;
                break;
            }

            ok(GrB_Vector_eWiseMult_BinaryOp(intersection1.get(), NULL, NULL, GxB_PAIR_BOOL, next1.get(),
                                             next2.get() /*prev next2*/, NULL));

            GrB_Index intersection1_nvals = GBxx_nvals(intersection1);
            if (intersection1_nvals > 0) {
                distance = level * 2 - 1;
                break;
            }

            ok(GrB_vxm(next2.get(), seen2.get(), NULL, GxB_ANY_PAIR_BOOL, next2.get(), A, GrB_DESC_RSC));
            ok(GrB_Vector_eWiseMult_BinaryOp(intersection2.get(), NULL, NULL, GxB_PAIR_BOOL, next1.get(),
                                             next2.get() /*current next2*/, NULL));

            GrB_Index intersection2_nvals = GBxx_nvals(intersection2);
            if (intersection2_nvals > 0) {
                distance = level * 2;
                break;
            }

            GrB_Index next2nvals = GBxx_nvals(next2);
#ifdef PRINT_EXTRA_COMMENTS
            next2_max = std::max(next2_max, next2nvals);
#endif
            if (next2nvals == 0) {
                distance = -1;
                break;
            }

            ok(GrB_Vector_eWiseAdd_BinaryOp(seen1.get(), NULL, NULL, GrB_LOR, seen1.get(), next1.get(), NULL));
            ok(GrB_Vector_eWiseAdd_BinaryOp(seen2.get(), NULL, NULL, GrB_LOR, seen2.get(), next2.get(), NULL));
        }
        add_comment_if_on("next1_max", std::to_string(next1_max));
        add_comment_if_on("next2_max", std::to_string(next2_max));
        add_comment_if_on("seen1", std::to_string(GBxx_nvals(seen1)));
        add_comment_if_on("seen2", std::to_string(GBxx_nvals(seen2)));
        add_comment_if_on("level", std::to_string(level));

        std::string result_str, comment_str;
        result_str = std::to_string(distance);
        return {result_str, comment_str};
    }

    std::array<std::string, 3> parameter_names() override {
        return {"p1_id", "p2_id", "comment_lower_limit"};
    }

public:
    int getQueryId() const override {
        return 1;
    }

    Query1(BenchmarkParameters const &benchmark_parameters, ParameterType query_params, QueryInput const &input)
            : Query(benchmark_parameters, std::move(query_params), input) {
        uint64_t p1_id, p2_id;

        std::tie(p1_id, p2_id, comment_lower_limit) = queryParams;

        p1 = input.persons.idToIndex(p1_id);
        p2 = input.persons.idToIndex(p2_id);
    }
};
