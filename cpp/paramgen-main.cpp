#include <iostream>
#include <memory>
#include <random>
#include "gb_utils.h"
#include "query-parameters.h"

class QueryParamGen {
protected:
    QueryInput const &input;
    std::mt19937_64 &randomEngine;
public:
    QueryParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : input(input), randomEngine(random_engine) {}
};

class Query2ParamGen : QueryParamGen {
    std::uniform_int_distribution<size_t> personDist;
    std::uniform_int_distribution<int> topKDist;

public:
    Query2ParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : QueryParamGen(input, random_engine),
              personDist(0, input.persons.size() - 1), topKDist(3, 7) {}

    std::tuple<int, std::string> operator()() {
        int top_k = topKDist(randomEngine);
        time_t birthday_limit = input.persons.vertices[personDist(randomEngine)].birthday;

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
    Query2ParamGen q2_gen{*input, random_engine};

    for (int i = 0; i < 100; ++i) {
        auto[k, d] = q2_gen();
        q2_file << k << ',' << d << std::endl;
    }

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
