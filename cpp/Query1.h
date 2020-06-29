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

    void advance_wavefront(GrB_Matrix HasCreator, GrB_Matrix HasCreatorTransposed,  GrB_Matrix ReplyOf, GrB_Matrix ReplyOfTransposed,  GrB_Matrix Knows, GrB_Vector frontier, GrB_Vector next, GrB_Vector seen, GrB_Index n) {
        if (comment_lower_limit == -1) {
            ok(GrB_vxm(next, seen, NULL, GxB_ANY_PAIR_BOOL, frontier, Knows, GrB_DESC_RC));
        } else {
            auto limit = GB(GxB_Scalar_new, GrB_INT64);
            ok(GxB_Scalar_setElement_INT64(limit.get(), comment_lower_limit));

            // build selection matrix based on the frontier's content
            GrB_Matrix Sel;
            GrB_Matrix_new(&Sel, GrB_BOOL, n, n);

            GrB_Index nvals1;
            GrB_Vector_nvals(&nvals1, frontier);
            GrB_Index *I = (GrB_Index*) (LAGraph_malloc(nvals1, sizeof(GrB_Index)));
            bool *X = (bool*) (LAGraph_malloc(nvals1, sizeof(bool)));

            GrB_Index nvals2 = nvals1;
            ok(GrB_Vector_extractTuples_BOOL(I, X, &nvals2, frontier));
            assert(nvals1 == nvals2);
            GrB_Matrix_build_BOOL(Sel, I, I, X, nvals1, GrB_LOR);

//            ok(GrB_mxm(Sel, NULL, NULL, GxB_ANY_PAIR_BOOL, Sel, Knows, NULL));

            GrB_Matrix M2;
            ok(GrB_Matrix_new(&M2, GrB_BOOL, n, input.comments.size()));
            GrB_mxm(M2, NULL, NULL, GxB_ANY_PAIR_BOOL, Sel, HasCreatorTransposed, NULL);

            // direction 1
            GrB_Matrix M3a;
            ok(GrB_Matrix_new(&M3a, GrB_UINT64, n, input.comments.size()));
            ok(GrB_mxm(M3a, NULL, NULL, GrB_PLUS_TIMES_SEMIRING_UINT64, M2, ReplyOf, NULL));

            GrB_Matrix Interactions1;
            ok(GrB_Matrix_new(&Interactions1, GrB_UINT64, n, n));
            ok(GrB_mxm(Interactions1, Knows, NULL, GrB_PLUS_TIMES_SEMIRING_UINT64, M3a, HasCreator, NULL));


#ifndef NDEBUG
//            GxB_Matrix_fprint(Interactions1, "Interactions1 before select", GxB_SUMMARY, stdout);
#endif
//            ok(GxB_Matrix_select(Interactions1, NULL, NULL, GxB_GT_THUNK, Interactions1, limit.get(), NULL));
#ifndef NDEBUG
//            GxB_Matrix_fprint(Interactions1, "Interactions1 after select", GxB_SUMMARY, stdout);
#endif

            // direction 2
            GrB_Matrix M3b;
            ok(GrB_Matrix_new(&M3b, GrB_UINT64, n, input.comments.size()));
            ok(GrB_mxm(M3b, NULL, NULL, GrB_PLUS_TIMES_SEMIRING_UINT64, M2, ReplyOfTransposed, NULL));


            GrB_Matrix Interactions2;
            ok(GrB_Matrix_new(&Interactions2, GrB_UINT64, n, n));
            ok(GrB_mxm(Interactions2, Interactions1, GrB_NULL, GrB_PLUS_TIMES_SEMIRING_UINT64, M3b, HasCreator, NULL));


#ifndef NDEBUG
            GxB_Matrix_fprint(Interactions2, "Interactions2 before select", GxB_SUMMARY, stdout);
#endif
#ifndef NDEBUG
//            GxB_Matrix_fprint(Interactions2, "Interactions2 after select", GxB_SUMMARY, stdout);
#endif


//#ifndef NDEBUG
//            GxB_Matrix_fprint(Sel, "Sel", GxB_SUMMARY, stdout);
//            GxB_Matrix_fprint(M2, "M2", GxB_SUMMARY, stdout);
//            GxB_Matrix_fprint(M3a, "M3a", GxB_SUMMARY, stdout);
//            GxB_Matrix_fprint(M3b, "M3b", GxB_SUMMARY, stdout);
//            GxB_Matrix_fprint(Interactions1, "Interactions1", GxB_SUMMARY, stdout);
//            GxB_Matrix_fprint(Interactions2, "Interactions2", GxB_SUMMARY, stdout);
//#endif

            // Interactions1 = Interactions1 * Interactions2
            ok(GrB_Matrix_eWiseMult_BinaryOp(Interactions1, NULL, NULL, GrB_MIN_UINT64, Interactions1, Interactions2, NULL));
            ok(GxB_Matrix_select(Interactions1, NULL, NULL, GxB_GT_THUNK, Interactions1, limit.get(), NULL));
#ifndef NDEBUG
//            GxB_Matrix_fprint(Interactions1, "Interactions final", GxB_SUMMARY, stdout);
#endif
            ok(GrB_Matrix_reduce_BinaryOp(next, NULL, NULL, GxB_PAIR_BOOL, Interactions1, GrB_DESC_T0));

        }
    }

    std::tuple<std::string, std::string> initial_calculation() override {

#ifndef NDEBUG
        using namespace std::chrono;
        auto start1 = high_resolution_clock::now();
#endif

        GrB_Matrix A = input.knows.matrix.get();

#ifndef NDEBUG
        auto duration1 = round<microseconds>(high_resolution_clock::now() - start1);
        auto start2 = high_resolution_clock::now();

        ok(GxB_Matrix_fprint(A, "personToPersonFiltered", GxB_SUMMARY, stdout));
#endif

        GrB_Index n;
        ok(GrB_Matrix_nrows(&n, A));


        GBxx_Object<GrB_Vector> frontier1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> frontier2 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> next1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> next2 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> intersection1 = GB(GrB_Vector_new, GrB_BOOL, n);
        GBxx_Object<GrB_Vector> intersection2 = GB(GrB_Vector_new, GrB_BOOL, n);

        ok(GrB_Vector_setElement_BOOL(frontier1.get(), true, p1));
        ok(GrB_Vector_setElement_BOOL(frontier2.get(), true, p2));
        GBxx_Object<GrB_Vector> seen1 = GB(GrB_Vector_dup, frontier1.get());
        GBxx_Object<GrB_Vector> seen2 = GB(GrB_Vector_dup, frontier2.get());

        int distance;
        if (p1 == p2) {
            distance = 0;
        } else {

            GrB_Matrix HCT, ROT;
            if (comment_lower_limit == -1) {
                HCT = NULL;
                ROT = NULL;
            } else {
                ok(GrB_Matrix_new(&HCT, GrB_UINT64, n, input.comments.size()));
                ok(GrB_transpose(HCT, NULL, NULL, input.hasCreator.matrix.get(), NULL));
                ok(GrB_Matrix_new(&ROT, GrB_UINT64, input.comments.size(), input.comments.size()));
                ok(GrB_transpose(ROT, NULL, NULL, input.replyOf.matrix.get(), NULL));
            }

            for (GrB_Index level = 1; level < n / 2 + 1; level++) {
                advance_wavefront(input.hasCreator.matrix.get(), HCT, input.replyOf.matrix.get(), ROT, input.knows.matrix.get(), frontier1.get(), next1.get(), seen1.get(), n);

                GrB_Index next1nvals;
                ok(GrB_Vector_nvals(&next1nvals, next1.get()));
                if (next1nvals == 0) {
                    distance = -1;
                    break;
                }

                ok(GrB_Vector_eWiseMult_BinaryOp(intersection1.get(), NULL, NULL, GrB_LAND, next1.get(), frontier2.get(), NULL));

                GrB_Index intersection1_nvals;
                ok(GrB_Vector_nvals(&intersection1_nvals, intersection1.get()));
                if (intersection1_nvals > 0) {
                    distance = level * 2 - 1;
                    break;
                }

//                ok(GrB_vxm(next2.get(), seen2.get(), NULL, GxB_ANY_PAIR_BOOL, frontier2.get(), A, GrB_DESC_RC));
                advance_wavefront(input.hasCreator.matrix.get(), HCT, input.replyOf.matrix.get(), ROT, input.knows.matrix.get(), frontier2.get(), next2.get(), seen2.get(), n);

                ok(GrB_Vector_eWiseMult_BinaryOp(intersection2.get(), NULL, NULL, GrB_LAND, next1.get(), next2.get(), NULL));

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

                frontier1 = GB(GrB_Vector_dup, next1.get());
                frontier2 = GB(GrB_Vector_dup, next2.get());
            }
        }

#ifndef NDEBUG
        auto duration2 = round<microseconds>(high_resolution_clock::now() - start2);
        std::cout << "induced subgraph: " << duration1.count()
                  << " bfs: "             << duration2.count() <<'\n';

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
