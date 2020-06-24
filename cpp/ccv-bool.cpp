#include "ccv.h"
#include "assert.h"


inline __attribute__((always_inline))
void create_diagonal_matrix(GrB_Matrix D) {
    GrB_Index n;
    ok(GrB_Matrix_ncols(&n, D));

    std::unique_ptr<GrB_Index[]> I{new GrB_Index[n]};
    std::unique_ptr<bool[]> X{new bool[n]};

    int nthreads = GlobalNThreads;
    nthreads = std::min<size_t>(n / 4096, nthreads);
    nthreads = std::max(nthreads, 1);
#pragma omp parallel for num_threads(nthreads) schedule(static)
    for (GrB_Index k = 0; k < n; k++) {
        I[k] = k;
        X[k] = true;
    }
    ok(GrB_Matrix_build_BOOL(D, I.get(), I.get(), X.get(), n, GrB_LOR));
}

// TODO: mapping comes from LAGraph (C code) and needs to be freed
std::tuple<GBxx_Object<GrB_Vector>, std::unique_ptr<GrB_Index[]>> compute_ccv_bool(GrB_Matrix A) {
    // initializing unary operator for nextCount

    GrB_Index n;
    ok(GrB_Matrix_nrows(&n, A));
    {
        GrB_Index ncols;
        ok(GrB_Matrix_ncols(&ncols, A));
        assert(n == ncols); // TODO replace with proper input check
    }

    GBxx_Object<GrB_Matrix> next = GB(GrB_Matrix_new, GrB_BOOL, n, n);

    GBxx_Object<GrB_Vector> nextCount = GB(GrB_Vector_new, GrB_UINT64, n);
    GBxx_Object<GrB_Vector> sp = GB(GrB_Vector_new, GrB_UINT64, n);
    GBxx_Object<GrB_Vector> compsize = GB(GrB_Vector_new, GrB_UINT64, n);
    GBxx_Object<GrB_Vector> ccv_result = GB(GrB_Vector_new, GrB_FP64, n);

    // initialize next and seen matrices: to compute closeness centrality, start off with a diagonal
    create_diagonal_matrix(next.get());
    GBxx_Object<GrB_Matrix> seen = GB(GrB_Matrix_dup, next.get());

//    // initialize
//    bool push = true; // TODO: add heuristic
//    //Heuristic
//    float threshold = 0.015;
//    GrB_Index frontier_nvals, matrix_nrows, source_nrows;
//    ok(GrB_Matrix_nvals(&frontier_nvals, frontier.get()));
//    ok(GrB_Matrix_nrows(&matrix_nrows, A));
//    //This should be the amount of bfs traversals, but since we traverse from all
//    //nodes, this equals A.nrows
//    ok(GrB_Matrix_nrows(&source_nrows, A));
//    float r, r_before;
//    r_before = (float) frontier_nvals / (float) (matrix_nrows * source_nrows);

//    GrB_Matrix C = NULL;
//    GrB_Index* mapping = NULL;
//    LAGraph_reorder_vertices(&C, &mapping, A, false);
//    A = C;

    // traversal
    for (GrB_Index level = 1; level < n; level++) {
//        printf("========================= Level %2ld =========================\n\n", level);
        // next<!seen> = next * A
        ok(GrB_mxm(next.get(), seen.get(), NULL, GxB_ANY_PAIR_BOOL, next.get(), A, GrB_DESC_RC));

        GrB_Index next_nvals;
        ok(GrB_Matrix_nvals(&next_nvals, next.get()));

        if (next_nvals == 0) {
//            printf("no new vertices found\n");
            break;
        }
        // nextCount = reduce(next'))
        ok(GrB_Matrix_reduce_Monoid(nextCount.get(), NULL, NULL, GxB_PLUS_UINT64_MONOID, next.get(), GrB_DESC_T0));

        // seen = seen | next
        ok(GrB_Matrix_eWiseAdd_BinaryOp(seen.get(), NULL, NULL, GrB_LOR, seen.get(), next.get(), NULL));

        // sp += (nextCount * level)
        //   nextCount * level is expressed as nextCount *= level_v
        ok(GrB_Vector_apply_BinaryOp1st_UINT64(nextCount.get(), NULL, NULL, GrB_TIMES_UINT64, level, nextCount.get(), NULL));
        ok(GrB_Vector_eWiseAdd_BinaryOp(sp.get(), NULL, NULL, GrB_PLUS_UINT64, sp.get(), nextCount.get(), NULL));
    }
    // compsize = reduce(seen, row)
    ok(GrB_Matrix_reduce_Monoid(compsize.get(), NULL, NULL, GxB_PLUS_UINT64_MONOID, seen.get(), GrB_DESC_T0));

    // compute the closeness centrality value:
    //
    //          (C(p)-1)^2
    // CCV(p) = ----------
    //          (n-1)*s(p)

    // all vectors are dense therefore eWiseAdd and eWiseMult are the same
    // C(p)-1
    ok(GrB_Vector_apply_BinaryOp2nd_UINT64(compsize.get(), NULL, NULL, GrB_MINUS_UINT64, compsize.get(), 1, NULL));
    // (C(p)-1)^2
    ok(GrB_Vector_eWiseMult_BinaryOp(compsize.get(), NULL, NULL, GrB_TIMES_UINT64, compsize.get(), compsize.get(), NULL));

    ok(GrB_Vector_apply_BinaryOp2nd_UINT64(sp.get(), NULL, NULL, GrB_TIMES_UINT64, sp.get(), n-1, NULL));
    ok(GrB_Vector_eWiseMult_BinaryOp(ccv_result.get(), NULL, NULL, GrB_DIV_FP64, compsize.get(), sp.get(), NULL));

    return std::tuple<GBxx_Object<GrB_Vector>, std::unique_ptr<GrB_Index[]>>{std::move(ccv_result), nullptr};
}
