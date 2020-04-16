#include <vector>
#include "gb_utils.h"

int main() {
    ok(LAGraph_init());

    GBxx_Object<GrB_Matrix> mx = GB(GrB_Matrix_new, GrB_BOOL, 4, 3);

    std::vector<GrB_Index> src_indices{0, 1, 3};
    std::vector<GrB_Index> trg_indices{1, 0, 2};

    ok(GrB_Matrix_build_BOOL(mx.get(),
                             src_indices.data(), trg_indices.data(),
                             array_of_true(src_indices.size()).get(),
                             src_indices.size(), GrB_LOR));
    WriteOutDebugMatrix(mx.get(), "matrix");

    GBxx_Object<GrB_Vector> vec = GB(GrB_Vector_new, GrB_BOOL, 4);
    ok(GrB_Vector_build_BOOL(vec.get(),
                             src_indices.data(),
                             array_of_true(src_indices.size()).get(),
                             src_indices.size(), GrB_LOR));
    WriteOutDebugVector(vec.get(), "vector");

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
