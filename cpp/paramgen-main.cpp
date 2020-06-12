#include <iostream>
#include <memory>
#include <random>
#include <filesystem>
#include "gb_utils.h"
#include "utils.h"
#include "query-parameters.h"

class QueryParamGen {
protected:
    QueryInput const &input;
    std::mt19937_64 &randomEngine;
public:
    QueryParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : input(input), randomEngine(random_engine) {}

    virtual void printTo(std::vector<std::unique_ptr<Printer>>& printers) = 0;

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

        uint64_t p1_id = input.persons.vertexIds[p1_index];
        uint64_t p2_id = input.persons.vertexIds[p2_index];

        return {p1_id, p2_id, comment_lower_limit};
    }

    void printTo(std::vector<std::unique_ptr<Printer>>& printers) override {
        auto[p1_id, p2_id, comment_lower_limit] = (*this)();
        for(auto& printer: printers) {
            printer->print(p1_id, p2_id, comment_lower_limit);
        }
    }
};

class Query2ParamGen : public QueryParamGen {
    std::uniform_int_distribution<size_t> personDist;
    std::uniform_int_distribution<int> topKDist;

public:
    Query2ParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : QueryParamGen(input, random_engine),
              personDist(0, input.personsWithBirthdays.size() - 1), topKDist(3, 7) {}

    std::tuple<int, std::string> operator()() {
        int top_k = topKDist(randomEngine);
        time_t birthday_limit = input.personsWithBirthdays.birthdays[personDist(randomEngine)];

        return {top_k, timestampToString(birthday_limit, DateFormat)};
    }

    void printTo(std::vector<std::unique_ptr<Printer>>& printers) override {
        auto[top_k, birthday_limit] = (*this)();
        for(auto& printer: printers) {
            printer->print(top_k, birthday_limit);
        }
    }
};

class Query3ParamGen : public QueryParamGen {
    std::uniform_int_distribution<int> topKDist;
    std::uniform_int_distribution<int> maxHopDist;
    std::uniform_int_distribution<size_t> placeDist;

public:
    Query3ParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : QueryParamGen(input, random_engine),
              topKDist(3, 7), maxHopDist(2, 6), placeDist(0, input.places.size() - 1) {}

    std::tuple<int, int, std::string> operator()() {
        int top_k_limit = topKDist(randomEngine);
        int maximum_hop_count = maxHopDist(randomEngine);
        std::string place_name = input.places.names[placeDist(randomEngine)];

        return {top_k_limit, maximum_hop_count, place_name};
    }

    void printTo(std::vector<std::unique_ptr<Printer>>& printers) override {
        auto[top_k_limit, maximum_hop_count, place_name] = (*this)();
        for(auto& printer: printers) {
            printer->print(top_k_limit, maximum_hop_count, place_name);
        }
    }
};

class Query4ParamGen : public QueryParamGen {
    std::uniform_int_distribution<int> topKDist;
    std::uniform_int_distribution<size_t> tagDist;

public:
    Query4ParamGen(QueryInput const &input, std::mt19937_64 &random_engine)
            : QueryParamGen(input, random_engine),
              topKDist(3, 7), tagDist(0, input.tags.size() - 1) {}

    std::tuple<int, std::string> operator()() {
        int top_k_limit = topKDist(randomEngine);
        std::string tag_name = input.tags.names[tagDist(randomEngine)];

        return {top_k_limit, tag_name};
    }

    void printTo(std::vector<std::unique_ptr<Printer>>& printers) override {
        auto[top_k_limit, tag_name] = (*this)();
        for(auto& printer: printers) {
            printer->print(top_k_limit, tag_name);
        }
    }
};

int main(int argc, char **argv) {
    BenchmarkParameters parameters = parse_benchmark_params(0, nullptr);
    ok(LAGraph_init());

    std::filesystem::create_directories(parameters.ParamsPath);

    std::unique_ptr<QueryInput> input = std::make_unique<QueryInput>(parameters);

    uint64_t seed = 698788706;
    std::mt19937_64 random_engine{seed};

    const std::string file_prefix = "query";
    const std::string csv_extension = ".csv";
    const std::string txt_extension = ".txt";
    const std::string txt_separator = ", ";

    std::vector<std::unique_ptr<QueryParamGen>> param_generators;
    param_generators.emplace_back(std::move(std::make_unique<Query1ParamGen>(*input, random_engine)));
    param_generators.emplace_back(std::move(std::make_unique<Query2ParamGen>(*input, random_engine)));
    param_generators.emplace_back(std::move(std::make_unique<Query3ParamGen>(*input, random_engine)));
    param_generators.emplace_back(std::move(std::make_unique<Query4ParamGen>(*input, random_engine)));

    for (int generator_idx = 0; generator_idx < param_generators.size(); ++generator_idx) {
        random_engine.seed(seed);

        const std::string query_number = std::to_string(generator_idx + 1);
        const std::string common_path{parameters.ParamsPath + file_prefix + query_number};

        std::vector<std::unique_ptr<Printer>> printers;
        printers.reserve(2);
        printers.emplace_back(std::make_unique<Printer>(common_path + csv_extension));
        printers.emplace_back(std::make_unique<Printer>(common_path + txt_extension, "query" + query_number + "(", ")", txt_separator));        

        for (int i = 0; i < 100; ++i) {
            param_generators[generator_idx]->printTo(printers);
        }
    }

    // Cleanup
    ok(LAGraph_finalize());

    return 0;
}
