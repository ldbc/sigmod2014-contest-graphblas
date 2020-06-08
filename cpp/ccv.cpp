#include "ccv.h"

#include "assert.h"

#define LAGRAPH_FREE_ALL                            \
{                                                   \
}

uint64_t extract(const GrB_Matrix A, const GrB_Index i, const GrB_Index j) {
    uint64_t x;
    GrB_Info info = GrB_Matrix_extractElement_UINT64(&x, A, i, j);
    if (info == GrB_NO_VALUE) {
        return 0xcccccccccccccccc;
    }
    return x;
}

uint64_t extract_v(const GrB_Vector v, const GrB_Index i) {
    uint64_t x;
    GrB_Info info = GrB_Vector_extractElement_UINT64(&x, v, i);
    if (info == GrB_NO_VALUE) {
        return 9999999;
    }
    return x;
}

void print_bit_matrix(const GrB_Matrix A) {
    GrB_Index nrows, ncols;
    GrB_Matrix_nrows(&nrows, A);
    GrB_Matrix_ncols(&ncols, A);
    for (GrB_Index i = 0; i < nrows; i++) {
        printf("%4ld:", i);
        for (GrB_Index j = 0; j < ncols; j++) {
            printf(" %016lx", extract(A, i, j));
        }
        printf("\n");
    }
    printf("\n");
}

void print_bit_matrices(const GrB_Matrix frontier, const GrB_Matrix next, const GrB_Matrix seen, const GrB_Vector next_popcount, const GrB_Vector sp) {
    GrB_Index nrows, ncols;
    GrB_Matrix_nrows(&nrows, frontier);
    GrB_Matrix_ncols(&ncols, frontier);
    printf("          frontier             next               seen         next_popcount    sp\n");
    for (GrB_Index i = 0; i < nrows; i++) {
        printf("%4ld:", i);
        for (GrB_Index j = 0; j < ncols; j++) {
            printf(
                    " %016lx   %016lx   %016lx   %13ld   %3ld",
                    extract(frontier, i, j), extract(next, i, j), extract(seen, i, j), extract_v(next_popcount, i), extract_v(sp, i));
        }
        printf("\n");
    }
    printf("\n");
}

GrB_Info create_diagonal_bit_matrix(GrB_Matrix D) {
    GrB_Info info;

    GrB_Index n;
    GrB_Matrix_nrows(&n, D);

//    I = 0, 1, ..., n
//    J = 0, 0, ..., 0 [64], 1, 1, ..., 1 [64], ..., ceil(n/64)
//    X = repeat {b100..., b010..., b001..., ..., b...001} until we have n elements
    GrB_Index *I = (GrB_Index *) LAGraph_malloc(n, sizeof(GrB_Index));
    GrB_Index *J = (GrB_Index *) LAGraph_malloc(n, sizeof(GrB_Index));
    uint64_t  *X = (uint64_t *)  LAGraph_malloc(n, sizeof(uint64_t));

    // TODO: parallelize this
    for (GrB_Index k = 0; k < n; k++) {
        I[k] = k;
        J[k] = k/64;
        X[k] = 0x8000000000000000L >> (k%64);
    }
    GrB_Matrix_build_UINT64(D, I, J, X, n, GrB_BOR_UINT64);

    return info;
}

uint64_t next_popcount(uint64_t x)
{
    int c = 0;
    for (; x != 0; x >>= 1)
        if (x & 1)
            c++;
    return c;
}

void fun_sum_popcount (void *z, const void *x)
{
    (*((uint64_t *) z))  = next_popcount(* ((uint64_t *) x));
}

GrB_Info compute_ccv(GrB_Vector *ccv_handle, GrB_Matrix A) {
    GrB_Matrix frontier = NULL, next = NULL, seen = NULL, Seen_PopCount = NULL ;

    GrB_Matrix Next_PopCount;
    GrB_Vector next_popcount;

    GrB_Vector ones, n_minus_one, level_v, sp, compsize;

    // initializing unary operator for next_popcount
    GrB_UnaryOp op_popcount = NULL ;
    LAGRAPH_TRY_CATCH (GrB_UnaryOp_new(&op_popcount, fun_sum_popcount, GrB_UINT64, GrB_UINT64));
    GrB_Semiring BOR_FIRST = NULL, BOR_SECOND = NULL ;
    LAGRAPH_TRY_CATCH (GrB_Semiring_new(&BOR_FIRST, GxB_BOR_UINT64_MONOID, GrB_FIRST_UINT64));
    LAGRAPH_TRY_CATCH (GrB_Semiring_new(&BOR_SECOND, GxB_BOR_UINT64_MONOID, GrB_SECOND_UINT64));

    GrB_Index n;
    GrB_Matrix_nrows(&n, A);
    {
        GrB_Index ncols;
        GrB_Matrix_ncols(&ncols, A);
        assert(n == ncols); // TODO replace with proper input check
    }

    const GrB_Index bit_matrix_ncols = (n+63)/64;

    GrB_Matrix_new(&frontier, GrB_UINT64, n, bit_matrix_ncols);
    GrB_Matrix_new(&next, GrB_UINT64, n, bit_matrix_ncols);
    GrB_Matrix_new(&seen, GrB_UINT64, n, bit_matrix_ncols);
    GrB_Matrix_new(&Next_PopCount, GrB_UINT64, n, bit_matrix_ncols);
    GrB_Matrix_new(&Seen_PopCount, GrB_UINT64, n, bit_matrix_ncols);

    GrB_Vector_new(&next_popcount, GrB_UINT64, n);
    GrB_Vector_new(&ones, GrB_UINT64, n);
    GrB_Vector_new(&n_minus_one, GrB_UINT64, n);
    GrB_Vector_new(&level_v, GrB_UINT64, n);
    GrB_Vector_new(&sp, GrB_UINT64, n);
    GrB_Vector_new(&compsize, GrB_UINT64, n);
    GrB_Vector_new(ccv_handle, GrB_FP64, n);

    // initialize frontier and seen matrices: to compute closeness centrality, start off with a diagonal
    create_diagonal_bit_matrix(frontier);
    GrB_Matrix_dup(&seen, frontier);

    // initialize vectors
    GrB_Vector_assign_UINT64(ones, NULL, NULL, 1, GrB_ALL, n, NULL);
    GrB_Vector_assign_UINT64(n_minus_one, NULL, NULL, n-1, GrB_ALL, n, NULL);

    // initialize

    // traversal
    for (GrB_Index level = 1; level < n; level++) {
//        printf("========================= Level %2ld =========================\n\n", level);
        // level_v += 1
        GrB_Vector_eWiseAdd_BinaryOp(level_v, NULL, NULL, GrB_PLUS_UINT64, level_v, ones, NULL);

        // next = frontier * A
        bool push = true; // TODO: add heuristic
        if (push) {
            GrB_vxm((GrB_Vector)next, NULL, NULL, BOR_FIRST, (GrB_Vector)frontier, A, NULL); // TODO: remove incorrect pointer casts
        } else {
            GrB_mxv((GrB_Vector)next, NULL, NULL, BOR_SECOND, A, (GrB_Vector)frontier, NULL); // TODO: remove incorrect pointer casts
        }

        // next = next & ~seen
        // We need to use eWiseAdd to see the union of value but mask with next so that
        // zero elements do not get the value from ~seen.
        // The code previously dropped zero elements. DO NOT do that as it will render
        // seen[i] = 0000 (implicit value) and seen[j] = 1111 equivalent in not_seen
        /*
             (Next)&& ! Seen  =  Next
              1100      1010     0100
                 -      0001        -
              1111         -     1111
            ----------------------------
             (Next)&&(neg(Seen)) = Next
              1100      0101       0100
                 -      1110          -
              1111         -       1111
            ----------------------------
            neg: apply f(a)=~a on explicit values
            GrB_apply: C<Mask> = accum (C, op(A))
            mask: Next
            desc: default (do not replace) to keep values which don't have corresponding seen values
            accum: &
            op: ~
            Next<Next> &= ~Seen
            (Seen = Next)
         */

        GrB_Matrix_apply(next, next, GrB_BAND_UINT64, GrB_BNOT_UINT64, seen, NULL);

        GrB_Index next_nvals;
        GrB_Matrix_nvals(&next_nvals, next);
        if (next == 0) {
//            printf("no new vertices found\n");
            break;
        }
        // next_popCount = reduce(apply(popcount, next))
        GrB_Matrix_apply(Next_PopCount, NULL, NULL, op_popcount, next, NULL);
        GrB_Matrix_reduce_Monoid(next_popcount, NULL, NULL, GxB_PLUS_UINT64_MONOID, Next_PopCount, NULL);

        // seen = seen | next
        GrB_Matrix_eWiseAdd_BinaryOp(seen, NULL, NULL, GrB_BOR_UINT64, seen, next, NULL);

        // sp += (next_popcount * level)
        //   next_popcount * level is expressed as next_popcount *= level_v
        GrB_Vector_eWiseMult_BinaryOp(next_popcount, NULL, NULL, GrB_TIMES_UINT64, next_popcount, level_v, NULL);
        GrB_Vector_eWiseAdd_BinaryOp(sp, NULL, NULL, GrB_PLUS_UINT64, sp, next_popcount, NULL);

//        print_bit_matrices(frontier, next, seen, next_popcount, sp);

        // frontier = next
        GrB_Matrix_dup(&frontier, next);
    }
    // compsize = reduce(seen, row -> popcount(row))
    GrB_Matrix_apply(Seen_PopCount, NULL, NULL, op_popcount, seen, NULL);
    GrB_Matrix_reduce_Monoid(compsize, NULL, NULL, GxB_PLUS_UINT64_MONOID, Seen_PopCount, NULL);

    // compute the closeness centrality value:
    //
    //          (C(p)-1)^2
    // CCV(p) = ----------
    //          (n-1)*s(p)

    // all vectors are dense therefore eWiseAdd and eWiseMult are the same
    // C(p)-1
    GrB_Vector_eWiseAdd_BinaryOp(compsize, NULL, NULL, GrB_MINUS_UINT64, compsize, ones, NULL);
    // (C(p)-1)^2
    GrB_Vector_eWiseMult_BinaryOp(compsize, NULL, NULL, GrB_TIMES_UINT64, compsize, compsize, NULL);

    GrB_Vector_eWiseMult_BinaryOp(sp, NULL, NULL, GrB_TIMES_UINT64, n_minus_one, sp, NULL);
    GrB_Vector_eWiseMult_BinaryOp(*ccv_handle, NULL, NULL, GrB_DIV_FP64, compsize, sp, NULL);

//    GxB_print(compsize, GxB_SHORT);
//    GxB_print(sp, GxB_SHORT);

    return GrB_SUCCESS;
}
