#include <iostream>
#include <memory>
#include <omp.h>
#include "gb_utils.h"
#include "Query2.h"
#include "query-parameters.h"

std::unique_ptr<QueryInput> load(BenchmarkParameters const &parameters) {
    using namespace std::chrono;
    auto load_start = high_resolution_clock::now();

    std::unique_ptr<QueryInput> input = std::make_unique<QueryInput>(parameters);

    report_load(parameters, round<nanoseconds>(high_resolution_clock::now() - load_start));

    return input;
}

int main(int argc, char *argv[]) {
    BenchmarkParameters parameters = parse_benchmark_params(argc, argv);

    ok(LAGraph_init());

    if (parameters.ThreadsNum > 0)
        LAGraph_set_nthreads(parameters.ThreadsNum);
    std::cerr << "Threads: " << LAGraph_get_nthreads() << '/' << omp_get_max_threads() << std::endl;

    std::unique_ptr<QueryInput> input = load(parameters);

    std::vector<std::function<std::string(void)>> queriesToRun = getQueriesWithParameters(parameters, *input);

    for (auto const &task :queriesToRun) {
        task();
    }

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
