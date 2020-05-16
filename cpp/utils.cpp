#include <string>
#include <chrono>
#include <optional>
#include <iostream>

#include "utils.h"

using namespace std::string_literals;

std::string getenv_string(const char *name, const std::optional<std::string> &default_ = std::nullopt) {
    const char *value = std::getenv(name);
    if (value)
        return value;
    else if (default_)
        return default_.value();
    else
        throw std::runtime_error{"Missing environmental variable: "s + name};
}

BenchmarkParameters parse_benchmark_params(int argc, char *argv[]) {
    using namespace std::literals;

    BenchmarkParameters params;

    if (argc >= 4) {
        if (argv[2] != "PARAM"sv)
            throw std::runtime_error(
                    "Command line arguments should be: <CSV_FOLDER> PARAM <QUERY_ID> <QUERY_PARAMS>...");

        params.CsvPath = argv[1];
        params.Query = std::stoi(argv[3]);
        params.QueryParams = argv + 4;
        params.QueryParamsNum = argc - 4;
    } else {
        params.CsvPath = getenv_string("CsvPath", "../../csvs/o1k/");
        params.ParamsPath = getenv_string("ParamsPath", "../../params/o1k/");
        params.Query = std::stoi(getenv_string("Query", "0"));

        if (*params.ParamsPath.rbegin() != '/')
            params.ParamsPath += '/';
    }

    if (*params.CsvPath.rbegin() != '/')
        params.CsvPath += '/';

    params.RunIndex = getenv_string("RunIndex", "0");
    params.Tool = getenv_string("Tool", "cpp");
    params.ChangeSet = getenv_string("ChangeSet", "1");

    const char *ThreadsNum_str = std::getenv("ThreadsNum");
    if (ThreadsNum_str)
        params.ThreadsNum = std::stoi(ThreadsNum_str);

    return params;
}

void report_load(const BenchmarkParameters &parameters, std::chrono::nanoseconds runtime) {
    using namespace std::chrono;

    std::cout
            << 'q' << parameters.Query << CSV_SEPARATOR
            << round<microseconds>(runtime).count() << CSV_SEPARATOR;
}

void report_result(const BenchmarkParameters &parameters, std::chrono::nanoseconds runtime,
                   std::tuple<std::string, std::string> const &result_tuple) {
    using namespace std::chrono;

    auto const&[result, comment] = result_tuple;

    std::cout
            << round<microseconds>(runtime).count()
            #ifndef NDEBUG
            << CSV_SEPARATOR << result
            << CSV_SEPARATOR << comment
            #endif
            << std::endl;
}

time_t parseTimestamp(const char *timestamp_str, const char *timestamp_format) {
    std::istringstream birthday_stream{timestamp_str};

    std::tm t = {};
    if (!(birthday_stream >> std::get_time(&t, timestamp_format)))
        throw std::invalid_argument{"Cannot parse timestamp: "s + timestamp_str};

    // depending on current time zone
    // it's acceptable since we only use these for comparison
    return std::mktime(&t);
}

std::string timestampToString(std::time_t timestamp, const char *timestamp_format) {
    std::stringstream stream;
    std::tm tm = *std::localtime(&timestamp);
    stream << std::put_time(&tm, timestamp_format);

    return stream.str();
}
