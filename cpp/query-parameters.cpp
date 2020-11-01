#include "query-parameters.h"
#include "Query1.h"
#include "Query2.h"
#include "Query3.h"
#include "Query4.h"
#include <stdexcept>
#include <iostream>

template<typename QueryType, typename... ParameterT>
auto getQueryWrapper(BenchmarkParameters benchmark_parameters, QueryInput const &input) {
    return [=, &input](ParameterT &&...query_parameters, std::optional<std::string> expected_result = std::nullopt)
            -> std::function<std::string()> {
        return [=, &input]() -> std::string {
            auto[result, comment] = QueryType(benchmark_parameters, std::make_tuple(query_parameters...), input)
                    .initial();
            if (expected_result) {
                std::string expected_result_value = expected_result.value();

                std::size_t comment_starts = expected_result_value.find('%');
                if (comment_starts != std::string::npos) {
                    // right trim space
                    // https://stackoverflow.com/a/217605
                    expected_result_value.erase(
                            std::find_if(
                                    std::make_reverse_iterator(expected_result_value.begin() + comment_starts),
                                    expected_result_value.rend(),
                                    [](int ch) { return !std::isspace(ch); }).base(),
                            expected_result_value.end());
                }

                if (result != expected_result_value) {
                    std::ostringstream params_stream;
//                    ((params_stream << query_parameters << ", "), ...);
                    std::string params = params_stream.str();
                    // remove comma
                    if (params.length() >= 2)
                        params.erase(params.length() - 2);

                    throw std::runtime_error(
                            "Results of query (" + params + ") mismatch:\nExpected: \"" + expected_result_value +
                            "\"\nActual:   \"" +
                            result + '"');
                }
            }

            return result;
        };
    };
}

std::vector<std::function<std::string()>>
getQueriesWithParameters(BenchmarkParameters benchmark_parameters, QueryInput const &input) {
    auto query1 = getQueryWrapper<Query1, uint64_t, uint64_t, int>(benchmark_parameters, input);
    auto query2 = getQueryWrapper<Query2, int, std::string>(benchmark_parameters, input);
    auto query3 = getQueryWrapper<Query3, int, int, std::string>(benchmark_parameters, input);
    auto query4 = getQueryWrapper<Query4, int, std::string>(benchmark_parameters, input);

    if (benchmark_parameters.QueryParams) {
        switch (benchmark_parameters.Query) {
            case 1:
                return {query1(std::stoull(benchmark_parameters.QueryParams[0]),
                               std::stoull(benchmark_parameters.QueryParams[1]),
                               std::stoi(benchmark_parameters.QueryParams[2]))};
            case 2:
                return {query2(std::stoi(benchmark_parameters.QueryParams[0]),
                               benchmark_parameters.QueryParams[1])};
            case 3:
                return {query3(std::stoi(benchmark_parameters.QueryParams[0]),
                               std::stoi(benchmark_parameters.QueryParams[1]),
                               benchmark_parameters.QueryParams[2])};
            case 4:
                return {query4(std::stoi(benchmark_parameters.QueryParams[0]),
                               benchmark_parameters.QueryParams[1])};

            default:
                throw std::runtime_error("Unknown query: " + std::to_string(benchmark_parameters.Query));
        }
    } else {
        // TEST CASES
        // test place name lookup
        for (size_t placeIndex = 0; placeIndex < input.places.size(); ++placeIndex) {
            auto const &place_name_ref = input.places.names[placeIndex];
            GrB_Index result = input.places.findIndexByName(place_name_ref);
            if (placeIndex != result) {
                std::string const &nameAtResultPosition =
                        result < input.places.size() ? input.places.names[result] : "invalid index";

                // for duplicate names minimum index should be returned
                if (nameAtResultPosition != place_name_ref || result > placeIndex)
                    throw std::runtime_error(
                            "Place name lookup: indices mismatch:\nExpected: \"" + std::to_string(placeIndex)
                            + "\" (" + place_name_ref + ")\nActual:   \"" + std::to_string(result) +
                            "\" (" + nameAtResultPosition + ')');
            }
        }

        std::vector<std::function<std::string()>> vector{
// formatter markers: https://stackoverflow.com/a/19492318
// @formatter:off
            query1(786, 799, 1,	R"(4 % path 786-63-31-60-799 (other shortest paths may exist))"),

// @formatter:on

// Takes extremely long to compile!
//#include "../o1k-queries-cpp.txt"

        };

        return vector;
    }
}
