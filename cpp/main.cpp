#include <iostream>
#include <memory>
#include <omp.h>
#include "gb_utils.h"
#include "query-parameters.h"
#include "utils.h"

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
    GlobalNThreads = LAGraph_get_nthreads();
    std::cerr << "Threads: " << GlobalNThreads << '/' << omp_get_max_threads() << std::endl;

    auto queriesToRun = getQueriesWithParameters(parameters);

    std::unique_ptr<QueryInput> input = load(parameters);

    for (auto const &task :queriesToRun) {
        task(parameters, *input);
    }

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
