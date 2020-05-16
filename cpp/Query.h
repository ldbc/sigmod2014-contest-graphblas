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
    BenchmarkParameters parameters;
    ParameterType queryParams;
    QueryInput const &input;

    virtual std::tuple<std::string, std::string> initial_calculation() = 0;

public:
    Query(BenchmarkParameters parameters, ParameterType queryParams, QueryInput const &input)
            : parameters{std::move(parameters)}, queryParams{std::move(queryParams)}, input(input) {}

    std::tuple<std::string, std::string> initial() override {
        using namespace std::chrono;
        auto initial_start = high_resolution_clock::now();

        auto resultTuple = initial_calculation();

        report_result(parameters, round<nanoseconds>(high_resolution_clock::now() - initial_start), std::get<0>(resultTuple));

        return resultTuple;
    }
};
