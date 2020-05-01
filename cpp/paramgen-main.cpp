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

    virtual std::ostream &printTo(std::ostream &out) = 0;

    virtual ~QueryParamGen() = default;
};

class Query1ParamGen : public QueryParamGen {
    std::uniform_int_distribution<size_t> personDist;
    std::uniform_int_distribution<int> commentLowerLimitDist;

public:
    Query1ParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : QueryParamGen(input, random_engine),
              personDist(0, input.persons.size() - 1), commentLowerLimitDist(-1, 3) {}

    std::tuple<uint64_t, uint64_t, int> operator()() {
        int comment_lower_limit = commentLowerLimitDist(randomEngine);

        size_t p1_index = personDist(randomEngine);
        size_t p2_index;
        do {
            p2_index = personDist(randomEngine);
        } while (p1_index == p2_index);

        uint64_t p1_id = input.persons.vertices[p1_index].id;
        uint64_t p2_id = input.persons.vertices[p2_index].id;

        return {p1_id, p2_id, comment_lower_limit};
    }

    std::ostream &printTo(std::ostream &out) override {
        auto[p1_id, p2_id, comment_lower_limit] = (*this)();
        out << p1_id << SEPARATOR << p2_id << SEPARATOR << comment_lower_limit << std::endl;
        return out;
    }
};

class Query2ParamGen : public QueryParamGen {
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

    std::ostream &printTo(std::ostream &out) override {
        auto[top_k, birthday_limit] = (*this)();
        out << top_k << SEPARATOR << birthday_limit << std::endl;
        return out;
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

    std::vector<std::unique_ptr<QueryParamGen>> param_generators;
    param_generators.emplace_back(std::move(std::make_unique<Query1ParamGen>(*input, random_engine)));
    param_generators.emplace_back(std::move(std::make_unique<Query2ParamGen>(*input, random_engine)));

    for (int generator_idx = 0; generator_idx < param_generators.size(); ++generator_idx) {
        random_engine.seed(seed);

        std::ofstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string path{parameters.ParamsPath
                         + file_prefix + std::to_string(generator_idx + 1) + file_extenstion};
        file.open(path);

        for (int i = 0; i < 100; ++i) {
            param_generators[generator_idx]->printTo(file);
        }
    }

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
