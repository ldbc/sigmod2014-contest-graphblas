#pragma once

#include <stdexcept>
#include <vector>
#include <chrono>
#include <memory>
#include <iomanip>

extern "C" {
#include <GraphBLAS.h>
#include <LAGraph.h>
}

struct BenchmarkParameters {
    std::string ChangePath;
    std::string RunIndex;
    std::string Tool;
    std::string ChangeSet;
    std::string Query;
    int ThreadsNum = 0;
};

BenchmarkParameters parse_benchmark_params();

using score_type = std::tuple<uint64_t, time_t, GrB_Index>;

struct BenchmarkPhase {
    static const std::string Initialization;
    static const std::string Load;
    static const std::string Initial;
};

void report(const BenchmarkParameters &parameters, int iteration, const std::string &phase,
            std::chrono::nanoseconds runtime,
            std::optional<std::string> result_reversed_opt = std::nullopt);

// https://stackoverflow.com/a/26351760
template<typename V, typename... T>
constexpr auto array_of(T &&... t)
-> std::array<V, sizeof...(T)> {
    return {{std::forward<T>(t)...}};
}

inline const char *TimestampFormat = "%Y-%m-%d %H:%M:%S";
inline const char *DateFormat = "%Y-%m-%d";

time_t parseTimestamp(const char *timestamp_str, const char *timestamp_format);

template<typename UnaryOp>
auto transformComparator(const UnaryOp &op) {
    return [&](const auto &lhs, const auto &rhs) {
        return op(lhs) < op(rhs);
    };
}
