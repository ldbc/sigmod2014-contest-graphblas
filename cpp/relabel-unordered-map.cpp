#include <iostream>
#include <memory>
#include <random>
#include <filesystem>
#include <unordered_map>
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"
#include "relabel-common.h"
#include <omp.h>

int main(int argc, char **argv) {
    ok(LAGraph_init());

    // TODO: set threads here
    constexpr uint64_t nthreads = 12;

    // prepare array of IDs
    srand(0);

//  const GrB_Index nnodes =  5;
//  const GrB_Index nedges = 15;
    const GrB_Index nnodes = 100 * 1000 * 1000;
    const GrB_Index nedges = 200 * 1000 * 1000;

    std::vector<GrB_Index> vertex_ids = getVertexIds(nnodes);
    printf("\n");

    std::vector<GrB_Index> edge_srcs = getListOfIds(vertex_ids, 131, nedges, nthreads);
    std::vector<GrB_Index> edge_trgs = getListOfIds(vertex_ids, 199, nedges, nthreads);

    double tic[2];

    // build id <-> index mapping
    LAGraph_tic (tic);

    using Id2IndexMap = std::unordered_map<GrB_Index, GrB_Index>;
    std::vector<Id2IndexMap> id2IndexMaps;
    id2IndexMaps.resize(nthreads);
    if constexpr (nthreads > 1) {
        id2IndexMaps[0].reserve(nnodes * 4);
    } else {
        for(uint64_t index = 0; index < nthreads; ++index) {
            id2IndexMaps[index].reserve(nnodes / (nthreads - 1) * 2);            
        }
    }
    #pragma omp parallel for num_threads(nthreads) schedule(static)
    for(GrB_Index index = 0U; index < vertex_ids.size(); ++index) {
        int threadId = omp_get_thread_num();
        Id2IndexMap &threadMap = id2IndexMaps[threadId];
        threadMap.emplace(vertex_ids[index], index);
    }

    Id2IndexMap id2Index;
    id2Index.swap(id2IndexMaps[0]);

    printf("Merging....\n");

    if constexpr (nthreads > 1) {
        id2Index.reserve(nnodes * 4);
        for(uint64_t index = 1; index < nthreads; ++index) {
            id2Index.merge(std::move(id2IndexMaps[index]));            
        }
    }
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
        GrB_Index src_index = id2Index.at(edge_srcs[j]);
        GrB_Index trg_index = id2Index.at(edge_trgs[j]);
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
