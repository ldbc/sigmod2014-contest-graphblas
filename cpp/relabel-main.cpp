#include <iostream>
#include <memory>
#include <random>
#include <filesystem>
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"
#include "relabel-common.h"

// https://www.geeksforgeeks.org/binary-search/
GrB_Index binarySearch(GrB_Index arr[], GrB_Index l, GrB_Index r, GrB_Index x)
{
    if (r >= l) {
        GrB_Index mid = l + (r - l) / 2;

        // If the element is present at the middle
        // itself
        if (arr[mid] == x)
            return mid;

        // If element is smaller than mid, then
        // it can only be present in left subarray
        if (arr[mid] > x)
            return binarySearch(arr, l, mid - 1, x);

        // Else the element can only be present
        // in right subarray
        return binarySearch(arr, mid + 1, r, x);
    }

    // We reach here when element is not
    // present in array
    return -1;
}

int main(int argc, char **argv) {
    ok(LAGraph_init());

    GrB_Vector id2index = NULL;

    // TODO: set threads here
    constexpr uint64_t nthreads = 12;
    LAGraph_set_nthreads (nthreads) ;

    // prepare array of IDs
    srand(0);
    constexpr uint64_t idsSeed = 50U;

    // std::vector<GrB_Index> vertex_ids = getVertexIds(nnodes);
    // std::vector<GrB_Index> edge_srcs = getListOfIds(vertex_ids, 131, nedges, nthreads);
    // std::vector<GrB_Index> edge_trgs = getListOfIds(vertex_ids, 199, nedges, nthreads);

    // saveToFile(vertex_ids, "5_15_vertex.data");
    // saveToFile(edge_srcs, "5_15_edge_srcs.data");
    // saveToFile(edge_trgs, "5_15_edge_trgs.data");
    
    std::vector<GrB_Index> vertex_ids = readFromFile("vertex.data");
    std::vector<GrB_Index> edge_srcs = readFromFile("edge_srcs.data");
    std::vector<GrB_Index> edge_trgs = readFromFile("edge_trgs.data");

    printf("Reading is done\n");

    double tic[2];

    // build id <-> index mapping
    LAGraph_tic (tic);
    LAGraph_dense_relabel(NULL, NULL, &id2index, vertex_ids.data(), nnodes, NULL);
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
        // GrB_Vector_extractElement_UINT64(&src_index, id2index, edge_srcs[j]);
        // GrB_Vector_extractElement_UINT64(&trg_index, id2index, edge_trgs[j]);
        src_index = X[binarySearch(I, 0, nnodes, edge_srcs[j])];
        trg_index = X[binarySearch(I, 0, nnodes, edge_trgs[j])];
        sum += src_index + trg_index;
        // printf("%d -> %d ==> %d -> %d\n", edge_srcs[j], edge_trgs[j], src_index, trg_index);
    }
    double time2 = LAGraph_toc(tic);
    printf("Edge relabel time: %.2f\n", time2);
    fprintf(stderr, " Totally not usable value: %d\n", sum);
    printf("%d\t%d\t%.2f\t%.2f\n", vertexMapperThreads, edgeMapperThreads, time1, time2);

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
