#include <iostream>
#include <memory>
#include <random>
#include <filesystem>
#include <unordered_map>
#include "tbb/concurrent_hash_map.h"
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"
#include "relabel-common.h"
#include <omp.h>

int main(int argc, char **argv) {
    // TODO: set threads here
    constexpr uint64_t vertexMapperThreads = 12;
    constexpr uint64_t edgeMappperThreads = 12;

    // prepare array of IDs
    srand(0);

    // const GrB_Index nnodes =  500;
    // const GrB_Index nedges = 1500;
    const GrB_Index nnodes = 100 * 1000 * 1000;
    const GrB_Index nedges = 200 * 1000 * 1000;

    // std::vector<GrB_Index> vertex_ids = getVertexIds(nnodes);
    // std::vector<GrB_Index> edge_srcs = getListOfIds(vertex_ids, 131, nedges, nthreads);
    // std::vector<GrB_Index> edge_trgs = getListOfIds(vertex_ids, 199, nedges, nthreads);

    // saveToFile(vertex_ids, "5_15_vertex.data");
    // saveToFile(edge_srcs, "5_15_edge_srcs.data");
    // saveToFile(edge_trgs, "5_15_edge_trgs.data");
    
    std::vector<GrB_Index> vertex_ids = readFromFile("100m_200m_vertex.data");
    std::vector<GrB_Index> edge_srcs = readFromFile("100m_200m_edge_srcs.data");
    std::vector<GrB_Index> edge_trgs = readFromFile("100m_200m_edge_trgs.data");

    printf("\n");

    double tic[2];

    // build id <-> index mapping
    LAGraph_tic (tic);

    using Id2IndexMap = tbb::concurrent_hash_map<GrB_Index, GrB_Index>;
    Id2IndexMap id2Index;

    #pragma omp parallel for num_threads(vertexMapperThreads) schedule(dynamic)
    for(GrB_Index index = 0U; index < vertex_ids.size(); ++index) {
        Id2IndexMap::accessor accessor;
        id2Index.insert(accessor, vertex_ids[index]);
        accessor->second = index;
    }
    double time1 = LAGraph_toc(tic);
    printf("Vertex relabel time: %.2f\n", time1);

    // just to prevent compiler to optimize out
    GrB_Index sum = 0u;
    // remap edges
    LAGraph_tic (tic);
    #pragma omp parallel for num_threads(edgeMappperThreads) schedule(static)
    for (GrB_Index j = 0; j < nedges; j++) {
        Id2IndexMap::const_accessor accessor;
        id2Index.find(accessor, edge_srcs[j]);
        GrB_Index src_index = accessor->second;
        accessor.release();
        id2Index.find(accessor, edge_trgs[j]);
        GrB_Index trg_index = accessor->second;
        sum += src_index + trg_index;
        // printf("%d -> %d ==> %d -> %d\n", edge_srcs[j], edge_trgs[j], src_index, trg_index);
    }
    double time2 = LAGraph_toc(tic);
    printf("Edge relabel time: %.2f\n", time2);
    fprintf(stderr, " Totally not usable value: %d\n", sum);
    return 0;
}
