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
    constexpr uint64_t nthreads = 12;
    LAGraph_set_nthreads (nthreads) ;

    // prepare array of IDs
    srand(0);
    constexpr uint64_t idsSeed = 50U;

    // std::vector<uint64_t> vertex_ids = getVertexIds(nnodes);
    // std::vector<uint64_t> edge_srcs = getListOfIds(vertex_ids, 131, nedges, nthreads);
    // std::vector<uint64_t> edge_trgs = getListOfIds(vertex_ids, 199, nedges, nthreads);

    // saveToFile(vertex_ids, "5_15_vertex.data");
    // saveToFile(edge_srcs, "5_15_edge_srcs.data");
    // saveToFile(edge_trgs, "5_15_edge_trgs.data");
    
    std::vector<uint64_t> vertex_ids = readFromFile("vertex.data");
    std::vector<uint64_t> edge_srcs = readFromFile("edge_srcs.data");
    std::vector<uint64_t> edge_trgs = readFromFile("edge_trgs.data");

    printf("Reading is done\n");

    double tic[2];

    // build id <-> index mapping
    LAGraph_tic (tic);
    LAGraph_dense_relabel(NULL, NULL, &id2index, vertex_ids.data(), nnodes, NULL);
    uint64_t* I = (uint64_t*) LAGraph_malloc(nnodes, sizeof(uint64_t));
    uint64_t* X = (uint64_t*) LAGraph_malloc(nnodes, sizeof(uint64_t));
    uint64_t nnodes2 = nnodes;
    ok(GrB_Vector_extractTuples_UINT64(I, X, &nnodes2, id2index));
    double time1 = LAGraph_toc(tic);
    printf("Vertex relabel time: %.2f\n", time1);

    /*
    printf("I: ");
    for (uint64_t i = 0; i < nnodes; i++) printf("%d, ", I[i]);
    printf("\n");

    printf("X: ");
    for (uint64_t i = 0; i < nnodes; i++) printf("%d, ", X[i]);
    printf("\n");
     */

    // just to prevent compiler to optimize out
    uint64_t sum = 0u;
    // remap edges
    LAGraph_tic (tic);
#pragma omp parallel for num_threads(nthreads) schedule(static)
    for (uint64_t j = 0; j < nedges; j++) {
        uint64_t src_index, trg_index;
//         GrB_Vector_extractElement_UINT64(&src_index, id2index, edge_srcs[j]);
//         GrB_Vector_extractElement_UINT64(&trg_index, id2index, edge_trgs[j]);

        src_index = X[std::lower_bound(I, I + nnodes, edge_srcs[j]) - I];
        trg_index = X[std::lower_bound(I, I + nnodes, edge_trgs[j]) - I];
        sum += src_index + trg_index;
//        printf("%ld -> %ld ==> %ld -> %ld\n", edge_srcs[j], edge_trgs[j], src_index, trg_index);
    }
    double time2 = LAGraph_toc(tic);
    printf("Edge relabel time: %.2f\n", time2);
    fprintf(stderr, " Totally not usable value: %ld\n", sum);
    printf("LAGraph with STL binsearch,%ld,%ld,%.2f,%.2f\n", nthreads, nthreads, time1, time2);

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
