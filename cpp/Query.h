#pragma once

#include <chrono>
#include <utility>
#include <memory>
#include <map>
#include <string>
#include <stdexcept>
#include "input.h"
#include "utils.h"
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
#ifdef PRINT_EXTRA_COMMENTS
    std::map<std::string, std::string> comments;

    inline void add_comment_if_on(std::string &&key, std::string &&value) {
        auto[iter, newlyInserted] = comments.emplace(std::move(key), std::move(value));
        if (!newlyInserted)
            throw std::runtime_error{iter->first + " already exists."};
    }

#else
#define add_comment_if_on(expr) static_cast<void>(0)
#endif


    virtual std::tuple<std::string, std::string> initial_calculation() = 0;

    virtual std::array<std::string, sizeof...(ParameterT)> parameter_names() = 0;

public:
    Query(BenchmarkParameters const &benchmark_parameters, ParameterType queryParams, QueryInput const &input)
            : benchmarkParameters{benchmark_parameters}, queryParams{std::move(queryParams)}, input(input) {}

    std::tuple<std::string, std::string> initial() override {
        using namespace std::chrono;
        auto initial_start = high_resolution_clock::now();

        auto result_tuple = initial_calculation();

#ifdef PRINT_EXTRA_COMMENTS
        std::string valueSeparator = "=", pairSeparator = "&";
        auto &comment = std::get<1>(result_tuple);
        comment = "\"comment" + valueSeparator + comment;

        comments.merge(getQueryParamsMap(queryParams, parameter_names()));

        for (const auto&[key, value] : comments) {
            comment += pairSeparator;
            comment += key;
            comment += valueSeparator;
            comment += value;
        }
        comment += '"';
#endif

        report_result(*this, benchmarkParameters, round<nanoseconds>(high_resolution_clock::now() - initial_start),
                      result_tuple);

        return result_tuple;
    }
};
