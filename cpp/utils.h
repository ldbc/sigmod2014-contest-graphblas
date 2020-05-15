#pragma once

#include <stdexcept>
#include <vector>
#include <chrono>
#include <memory>
#include <fstream>
#include <iomanip>

extern "C" {
#include <GraphBLAS.h>
#include <LAGraph.h>
}

struct BenchmarkParameters {
    std::string CsvPath;
    std::string ParamsPath;
    std::string RunIndex;
    std::string Tool;
    std::string ChangeSet;
    int Query;
    char const *const *QueryParams = nullptr;
    int QueryParamsNum = 0;
    int ThreadsNum = 0;
};

BenchmarkParameters parse_benchmark_params(int argc, char *argv[]);

using score_type = std::tuple<uint64_t, time_t, GrB_Index>;

void report_load(const BenchmarkParameters &parameters, std::chrono::nanoseconds runtime);

void report_result(const BenchmarkParameters &parameters, std::chrono::nanoseconds runtime, std::string const &result);

// https://stackoverflow.com/a/26351760
template<typename V, typename... T>
constexpr auto array_of(T &&... t)
-> std::array<V, sizeof...(T)> {
    return {{std::forward<T>(t)...}};
}

inline char const CSV_SEPARATOR = ',';
inline char const *CSV_SEPARATOR_STR = ",";

inline const char *TimestampFormat = "%Y-%m-%d %H:%M:%S";
inline const char *DateFormat = "%Y-%m-%d";

time_t parseTimestamp(const char *timestamp_str, const char *timestamp_format);

std::string timestampToString(std::time_t timestamp, const char *timestamp_format);

template<typename UnaryOp>
auto transformComparator(const UnaryOp &op) {
    return [&](const auto &lhs, const auto &rhs) {
        return op(lhs) < op(rhs);
    };
}

class Printer {
    std::ofstream m_output;
    const std::string m_prefix;
    const std::string m_postfix;
    const std::string m_separator;
public:
    Printer(const std::string &filePath,
            std::string prefix = "",
            std::string postfix = "",
            std::string separator = CSV_SEPARATOR_STR)
            : m_output(), m_prefix(std::move(prefix)), m_postfix(std::move(postfix)),
              m_separator(std::move(separator)) {
        m_output.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        m_output.open(filePath);
    }

    template<typename Arg, typename... Args>
    void print(Arg &&arg, Args &&... args) {
        m_output << m_prefix << std::forward<Arg>(arg);
        ((m_output << m_separator << std::forward<Args>(args)), ...);
        m_output << m_postfix << std::endl;
    }
};
