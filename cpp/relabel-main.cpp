#include <iostream>
#include <memory>
#include <random>
#include <filesystem>
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"
#include "relabel-common.h"

int main(int argc, char **argv) {
    ok(LAGraph_init());

    GrB_Vector id2index = NULL;

    // TODO: set threads here
    uint64_t nthreads = 8;
    LAGraph_set_nthreads (nthreads) ;

    // prepare array of IDs
    srand(0);

//  const GrB_Index nnodes =  5;
//  const GrB_Index nedges = 15;
    const GrB_Index nnodes =  2 * 1000 * 1000;
    const GrB_Index nedges = 100 * 1000 * 1000;

    GrB_Index* vertex_ids = (GrB_Index*) malloc(nnodes * sizeof(GrB_Index));
#pragma omp parallel for num_threads(nthreads) schedule(static)
    for (GrB_Index i = 0; i < nnodes; i++) {
        vertex_ids[i] = rand();
    }
    printf("\n");

    GrB_Index* edge_srcs = (GrB_Index*) malloc(nedges * sizeof(GrB_Index));
    GrB_Index* edge_trgs = (GrB_Index*) malloc(nedges * sizeof(GrB_Index));
#pragma omp parallel for num_threads(nthreads) schedule(static)
    for (GrB_Index j = 0; j < nedges; j++) {
        edge_srcs[j] = vertex_ids[fasthash(j * 131) % nnodes];
        edge_trgs[j] = vertex_ids[fasthash(j * 199) % nnodes];
        // edge_srcs[j] = vertex_ids[rand() % nnodes];
        // edge_trgs[j] = vertex_ids[rand() % nnodes];
    }

    double tic[2];

    // build id <-> index mapping
    LAGraph_tic (tic);
    LAGraph_dense_relabel(NULL, NULL, &id2index, vertex_ids, nnodes, NULL);
    GrB_Index* I = (GrB_Index*) LAGraph_malloc(nnodes, sizeof(GrB_Index));
    GrB_Index* X = (GrB_Index*) LAGraph_malloc(nnodes, sizeof(GrB_Index));
    GrB_Index nnodes2;
    GrB_Vector_extractTuples_UINT64(I, X, &nnodes2, id2index);
    double time1 = LAGraph_toc(tic);
    printf("Vertex relabel time: %.2f\n", time1);

    /*
    printf("I: ");
    for (GrB_Index i = 0; i < nnodes; i++) printf("%d, ", I[i]);
    printf("\n");

    printf("X: ");
    for (GrB_Index i = 0; i < nnodes; i++) printf("%d, ", X[i]);
    printf("\n");
     */

    // just to prevent compiler to optimize out
    GrB_Index sum = 0u;
    // remap edges
    LAGraph_tic (tic);
#pragma omp parallel for num_threads(nthreads) schedule(static)
    for (GrB_Index j = 0; j < nedges; j++) {
        GrB_Index src_index, trg_index;
        GrB_Vector_extractElement_UINT64(&src_index, id2index, edge_srcs[j]);
        GrB_Vector_extractElement_UINT64(&trg_index, id2index, edge_trgs[j]);
        sum += src_index + trg_index;
        // printf("%d -> %d ==> %d -> %d\n", edge_srcs[j], edge_trgs[j], src_index, trg_index);
    }
    double time2 = LAGraph_toc(tic);
    printf("Edge relabel time: %.2f\n", time2);
    fprintf(stderr, " Totally not usable value: %d\n", sum);

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
