#pragma once

#include <chrono>
#include <utility>
#include <memory>
#include "load.h"
#include "BaseQuery.h"
#include "gb_utils.h"

template<typename InputT>
class Query : public BaseQuery {
protected:
    BenchmarkParameters parameters;
    std::unique_ptr<InputT> input;
    inline static const int top_count = 3;

    static void add_score_to_toplist(std::vector<score_type> &top_scores, score_type score) {
        if (top_scores.size() < top_count || score > top_scores.front()) {
            top_scores.push_back(score);
            std::push_heap(top_scores.begin(), top_scores.end(), std::greater<>{});

            if (top_scores.size() > top_count) {
                std::pop_heap(top_scores.begin(), top_scores.end(), std::greater<>{});
                top_scores.pop_back();
            }
        }
    }

    static void sort_top_scores(std::vector<score_type> &top_scores) {
        std::sort_heap(top_scores.begin(), top_scores.end(), std::greater<>{});
    }

public:
    explicit Query(BenchmarkParameters parameters) : parameters{std::move(parameters)} {}

    void load() override {
        using namespace std::chrono;
        auto load_start = high_resolution_clock::now();

        input = std::make_unique<InputT>(parameters);

        report(parameters, 0, BenchmarkPhase::Load, round<nanoseconds>(high_resolution_clock::now() - load_start));
    }

    void initial() override {
        using namespace std::chrono;
        auto initial_start = high_resolution_clock::now();

        std::vector<uint64_t> top_scores_vector = initial_calculation();

        report(parameters, 0, BenchmarkPhase::Initial, round<nanoseconds>(high_resolution_clock::now() - initial_start),
               top_scores_vector);
    }

    virtual std::vector<uint64_t> initial_calculation() = 0;
};
