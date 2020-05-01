#include <iostream>
#include <memory>
#include <random>
#include "gb_utils.h"
#include "query-parameters.h"

class Query2Params {
    std::mt19937_64 &randomEngine;
    std::uniform_int_distribution<std::time_t> birthday_limit_dist;
    std::uniform_int_distribution<int> top_k_dist;

    static std::uniform_int_distribution<std::time_t> initBirthdayLimitDist(QueryInput const &input) {
        auto comparator = transformComparator([](const auto &val) { return val.birthday; });

        std::time_t min = std::min_element(input.persons.vertices.begin(), input.persons.vertices.end(),
                                           comparator)->birthday,
                max = std::max_element(input.persons.vertices.begin(), input.persons.vertices.end(),
                                       comparator)->birthday;

        return std::uniform_int_distribution<std::time_t>{min, max};
    }

public:
    Query2Params(QueryInput const &input, std::mt19937_64 &random_engine)
            : randomEngine(random_engine), birthday_limit_dist(initBirthdayLimitDist(input)), top_k_dist(3, 7) {}

    std::tuple<int, std::string> operator()() {
        int top_k = top_k_dist(randomEngine);
        time_t birthday_limit = birthday_limit_dist(randomEngine);

        return {top_k, timestampToString(birthday_limit, DateFormat)};
    }
};

int main(int argc, char **argv) {
    BenchmarkParameters parameters = parse_benchmark_params();
    ok(LAGraph_init());

    std::unique_ptr<QueryInput> input = std::make_unique<QueryInput>(parameters);

    uint64_t seed = 698788706;
    std::mt19937_64 random_engine{seed};

    const char *file_prefix = "query";
    const char *file_extenstion = ".csv";

    std::ofstream q2_file;
    q2_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    q2_file.open(parameters.ParamsPath + file_prefix + "2" + file_extenstion);
    Query2Params q2_gen{*input, random_engine};

    for (int i = 0; i < 100; ++i) {
        auto[k, d] = q2_gen();
        q2_file << k << ',' << d << std::endl;
    }

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
