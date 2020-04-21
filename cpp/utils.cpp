#include <string>
#include <chrono>
#include <optional>
#include <algorithm>
#include <iostream>

#include "utils.h"


    std::string getenv_string(const char *name, const std::optional<std::string> &default_ = std::nullopt) {
    const char *value = std::getenv(name);
    if (value)
        return value;
    else if (default_)
        return default_.value();
    else
        throw std::runtime_error{std::string{"Missing environmental variable: "} + name};
}

BenchmarkParameters parse_benchmark_params() {
    BenchmarkParameters params;

    params.ChangePath = getenv_string("ChangePath", "../sf1k-converted/");
    if (*params.ChangePath.rbegin() != '/')
        params.ChangePath += '/';

    params.RunIndex = getenv_string("RunIndex", "0");
    params.Tool = getenv_string("Tool", "CPP");
    params.ChangeSet = getenv_string("ChangeSet", "1");
    params.Query = getenv_string("Query", "QAll");

    const char *ThreadsNum_str = std::getenv("ThreadsNum");
    if (ThreadsNum_str)
        params.ThreadsNum = std::stoi(ThreadsNum_str);

    std::transform(params.Query.begin(), params.Query.end(), params.Query.begin(),
                   [](char c) { return std::toupper(c); });

    return params;
}

const std::string BenchmarkPhase::Initialization = "Initialization";
const std::string BenchmarkPhase::Load = "Load";
const std::string BenchmarkPhase::Initial = "Initial";

void report_info(const BenchmarkParameters &parameters, int iteration, const std::string &phase) {
    std::cout
            << parameters.Tool << ';'
            << parameters.Query << ';'
            << parameters.ChangeSet << ';'
            << parameters.RunIndex << ';'
            << iteration << ';'
            << phase << ';';
}

void
report(const BenchmarkParameters &parameters, int iteration, const std::string &phase, std::chrono::nanoseconds runtime,
       std::optional<std::string> result_opt) {
    report_info(parameters, iteration, phase);
    std::cout << "Time" << ';' << runtime.count() << std::endl;

    if (result_opt) {
        const auto &result = result_opt.value();

        report_info(parameters, iteration, phase);
        std::cout << "Elements" << ';' << result << std::endl;
    }
}

time_t parseTimestamp(const char *timestamp_str, const char *timestamp_format) {
    std::istringstream birthday_stream{timestamp_str};

    std::tm t = {};
    if (!(birthday_stream >> std::get_time(&t, timestamp_format)))
        throw std::invalid_argument{std::string{"Cannot parse timestamp: "} + timestamp_str};

    // depending on current time zone
    // it's acceptable since we only use these for comparison
    return std::mktime(&t);
}
