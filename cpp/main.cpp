#include <iostream>
#include <vector>
#include "gb_utils.h"
#include "Query2.h"

std::unique_ptr<BaseQuery> init_solution(BenchmarkParameters &parameters) {
    if (parameters.Query == "Q2")
        return std::make_unique<Query2>(parameters);

    throw std::runtime_error{"Unknown query: " + parameters.Query};
}

int main(int argc, char **argv) {
    BenchmarkParameters parameters = parse_benchmark_params();
    ok(LAGraph_init());

    std::unique_ptr<BaseQuery> solution = init_solution(parameters);

    solution->load();
    solution->initial();


    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
