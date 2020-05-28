#include "relabel-common.h"

#include <unordered_set>

constexpr uint64_t maxId = 1152921504606846976U;

uint64_t fasthash(uint64_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

constexpr uint64_t idSeed = 50U;

std::vector<GrB_Index> getVertexIds(uint64_t numberOfVertices) {
    uint64_t seed =  idSeed * numberOfVertices;
    std::unordered_set<GrB_Index> idsSet;
    while(idsSet.size() < numberOfVertices) {
        seed = fasthash(seed + idsSet.size());
        idsSet.insert(seed % maxId);
    }
    std::vector<GrB_Index> idsVector;
    idsVector.reserve(numberOfVertices);
    idsVector.insert(idsVector.begin(), idsSet.begin(), idsSet.end());
    return idsVector;
}

std::vector<GrB_Index> getListOfIds(const std::vector<GrB_Index>& vertexIds, uint64_t multiplier, uint64_t numberOfIndices, uint64_t numberOfThreads) {
    std::vector<GrB_Index> indices;
    indices.reserve(numberOfIndices);

    #pragma omp parallel for num_threads(numberOfThreads) schedule(static)
    for (GrB_Index j = 0; j < numberOfIndices; j++) {
        indices[j] = vertexIds[fasthash(j * multiplier) % vertexIds.size()];
    }

    return indices;
}