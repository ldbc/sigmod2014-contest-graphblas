#include <iostream>
#include <memory>
#include <random>
#include <filesystem>
#include <unordered_map>
#include "tbb/concurrent_unordered_map.h"
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"
#include "relabel-common.h"
#include <omp.h>

int main(int argc, char **argv) {
    // TODO: set threads here
    constexpr uint64_t edgeMappperThreads = 12;
#ifndef NUMBER_OF_VERTEX_MAPPER_THREADS
    #define NUMBER_OF_VERTEX_MAPPER_THREADS edgeMappperThreads
#endif
    constexpr uint64_t vertexMapperThreads = NUMBER_OF_VERTEX_MAPPER_THREADS;

    // prepare array of IDs
    srand(0);

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

    using Id2IndexMap = tbb::concurrent_unordered_map<GrB_Index, GrB_Index>;
    Id2IndexMap id2Index;

    #pragma omp parallel for num_threads(vertexMapperThreads) schedule(static)
    for(GrB_Index index = 0U; index < vertex_ids.size(); ++index) {
        id2Index.emplace(vertex_ids[index], index);
    }
    double time1 = LAGraph_toc(tic);
    printf("Vertex relabel time: %.2f\n", time1);

    // just to prevent compiler to optimize out
    GrB_Index sum = 0u;
    // remap edges
    LAGraph_tic (tic);
    #pragma omp parallel for num_threads(edgeMappperThreads) schedule(static)
    for (GrB_Index j = 0; j < nedges; j++) {
        GrB_Index src_index = id2Index.at(edge_srcs[j]);
        GrB_Index trg_index = id2Index.at(edge_trgs[j]);
        sum += src_index + trg_index;
        // printf("%d -> %d ==> %d -> %d\n", edge_srcs[j], edge_trgs[j], src_index, trg_index);
    }
    double time2 = LAGraph_toc(tic);
    printf("Edge relabel time: %.2f\n", time2);
    fprintf(stderr, " Totally not usable value: %d\n", sum);
    printf("%d\t%d\t%.2f\t%.2f\n", vertexMapperThreads, edgeMapperThreads, time1, time2);
    return 0;
}
