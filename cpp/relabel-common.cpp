#include "relabel-common.h"

#include <fstream>
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
    indices.resize(numberOfIndices);

    #pragma omp parallel for num_threads(numberOfThreads) schedule(static)
    for (GrB_Index j = 0; j < numberOfIndices; j++) {
        indices[j] = vertexIds[fasthash(j * multiplier) % vertexIds.size()];
    }

    return indices;
}

// these are not the best functions to save/read vectors from binary files, but definitely
// the quickest solution
void saveToFile(const std::vector<GrB_Index> &toSave, const std::string &filename) {
    std::ofstream output;
    output.open(filename, std::ios::binary | std::ios::out);
    output.write(reinterpret_cast<const char*>(toSave.data()), toSave.size() * sizeof(GrB_Index));
    output.close();
}

std::vector<GrB_Index> readFromFile(const std::string &filename) {
    std::ifstream input(filename, std::ios::binary | std::ios::ate);

    auto fileSize = input.tellg();
    const auto numberOfIds = fileSize / sizeof(GrB_Index);
    if (fileSize != numberOfIds * sizeof(GrB_Index)) {
        throw std::runtime_error("File size doesn't match the size of GrB_Index");
    }
    input.seekg(0, std::ios::beg);


    std::vector<GrB_Index> result;
    result.resize(numberOfIds);
    input.read(reinterpret_cast<char*>(result.data()), fileSize);
    input.close();

    return result;
}