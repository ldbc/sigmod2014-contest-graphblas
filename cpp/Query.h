#pragma once

#include <chrono>
#include <utility>
#include <memory>
#include "input.h"
#include "BaseQuery.h"
#include "gb_utils.h"

template<typename... ParameterT>
class Query : public BaseQuery {

public:
    using ParameterType = std::tuple<ParameterT...>;
protected:
    BenchmarkParameters const &benchmarkParameters;
    ParameterType queryParams;
    QueryInput const &input;

    virtual std::tuple<std::string, std::string> initial_calculation() = 0;

public:
    Query(BenchmarkParameters const &benchmark_parameters, ParameterType queryParams, QueryInput const &input)
            : benchmarkParameters{benchmark_parameters}, queryParams{std::move(queryParams)}, input(input) {}

    std::tuple<std::string, std::string> initial() override {
        using namespace std::chrono;
        auto initial_start = high_resolution_clock::now();

        auto result_tuple = initial_calculation();

        report_result(*this, benchmarkParameters, round<nanoseconds>(high_resolution_clock::now() - initial_start),
                      result_tuple);

        return result_tuple;
    }
};
