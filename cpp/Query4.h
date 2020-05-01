#pragma once

#include <queue>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <memory>
#include <set>
#include <cstdio>
#include <utility>
#include "utils.h"
#include "Query.h"
#include <cstdio>

class Query4 : public Query<int, std::string> {
    int topKLimit;
    std::string tagName;

    std::tuple<std::string, std::string> initial_calculation() override {
        ok(GxB_Matrix_fprint(input.knows.matrix.get(), "knows", GxB_SUMMARY, stdout));

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

        GBxx_Object<GrB_Vector> result = GB(GrB_Vector_new, GrB_BOOL, 3);
        ok(GrB_vxm(result.get(), GrB_NULL, GrB_NULL,
                   GxB_LOR_LAND_BOOL, vec.get(), mx.get(), GrB_NULL));
        WriteOutDebugVector(result.get(), "result");

        std::string result_str, comment_str;
        return {result_str, comment_str};
    }

public:
    Query4(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        std::tie(topKLimit, tagName) = queryParams;
    }
};
