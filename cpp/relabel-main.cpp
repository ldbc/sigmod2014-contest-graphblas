#include <iostream>
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"
#include "relabel-common.h"

int main(int argc, char **argv) {
    ok(LAGraph_init());

    GrB_Vector id2index = NULL;

    constexpr uint64_t nthreads = 1;
    LAGraph_set_nthreads (nthreads) ;

    // prepare array of IDs
    srand(0);
    constexpr uint64_t idsSeed = 50U;

    std::vector<GrB_Index> vertex_ids = readFromFile("vertex.data");
    std::vector<GrB_Index> edge_srcs = readFromFile("edge_srcs.data");
    std::vector<GrB_Index> edge_srcs2 = readFromFile("edge_srcs.data");
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


    printf("stl: %ld  \n", std::lower_bound(I, I + nnodes, edge_srcs[0]) - I   );
    printf("stl: %ld  \n", std::lower_bound(I, I + nnodes, edge_srcs[1]) - I   );
    printf("stl: %ld  \n", std::lower_bound(I, I + nnodes, edge_srcs[2]) - I   );

    for (int idx = 0; idx < 3; idx++) {
        printf("stl: %ld  \n", std::lower_bound(I, I + nnodes, edge_srcs2[idx]) - I );
    }

    ok(LAGraph_finalize());

    return 0;
}
