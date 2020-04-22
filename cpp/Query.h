#pragma once

#include <chrono>
#include <utility>
#include <memory>
#include "load.h"
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

//    static void add_score_to_toplist(std::vector<score_type> &top_scores, score_type score) {
//        if (top_scores.size() < top_count || score > top_scores.front()) {
//            top_scores.push_back(score);
//            std::push_heap(top_scores.begin(), top_scores.end(), std::greater<>{});
//
//            if (top_scores.size() > top_count) {
//                std::pop_heap(top_scores.begin(), top_scores.end(), std::greater<>{});
//                top_scores.pop_back();
//            }
//        }
//    }
//
//    static void sort_top_scores(std::vector<score_type> &top_scores) {
//        std::sort_heap(top_scores.begin(), top_scores.end(), std::greater<>{});
//    }

    virtual std::tuple<std::string, std::string> initial_calculation() = 0;

public:
    Query(BenchmarkParameters parameters, ParameterType queryParams, QueryInput const &input)
            : parameters{std::move(parameters)}, queryParams{std::move(queryParams)}, input(input) {}

    std::tuple<std::string, std::string> initial() override {
        using namespace std::chrono;
        auto initial_start = high_resolution_clock::now();

        auto resultTuple = initial_calculation();

        report(parameters, 0, BenchmarkPhase::Initial, round<nanoseconds>(high_resolution_clock::now() - initial_start),
               std::get<0>(resultTuple));

        return resultTuple;
    }
};
