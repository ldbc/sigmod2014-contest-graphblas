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
    constexpr uint64_t nThreads = 12;

    std::vector<GrB_Index> vertex_ids = getVertexIds(nnodes);
    std::vector<GrB_Index> edge_srcs = getListOfIds(vertex_ids, 131, nedges, nThreads);
    std::vector<GrB_Index> edge_trgs = getListOfIds(vertex_ids, 199, nedges, nThreads);

    saveToFile(vertex_ids, "vertex.data");
    saveToFile(edge_srcs, "edge_srcs.data");
    saveToFile(edge_trgs, "edge_trgs.data");

    return 0;
}
