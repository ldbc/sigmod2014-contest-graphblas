#include "query-parameters.h"
#include "Query1.h"
#include "Query2.h"
#include "Query3.h"
#include "Query4.h"
#include <stdexcept>

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

                if (result != expected_result_value)
                    throw std::runtime_error(
                            "Query result mismatches:\nExpected: \"" + expected_result_value + "\"\nActual:   \"" +
                            result + '"');
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
        std::vector<std::function<std::string()>> vector{
// formatter markers: https://stackoverflow.com/a/19492318
// @formatter:off
            query1(576, 400, -1, R"(3 % path 576-618-951-400 (other shortest paths may exist))"),
            query1(58, 402, 0, R"(3 % path 58-935-808-402 (other shortest paths may exist))"),
            query1(266, 106, -1, R"(3 % path 266-23-592-106 (other shortest paths may exist))"),
            query1(313, 523, -1, R"(-1 % path none)"),
            query1(858, 587, 1, R"(4 % path 858-46-31-162-587 (other shortest paths may exist))"),
            query1(155, 355, -1, R"(3 % path 155-21-0-355 (other shortest paths may exist))"),
            query1(947, 771, -1, R"(2 % path 947-625-771 (other shortest paths may exist))"),
            query1(105, 608, 3, R"(-1 % path none)"),
            query1(128, 751, -1, R"(3 % path 128-459-76-751 (other shortest paths may exist))"),
            query1(814, 641, 0, R"(3 % path 814-109-557-641 (other shortest paths may exist))"),

            query2(3, "1980-02-01", R"(Chiang_Kai-shek Augustine_of_Hippo Napoleon % component sizes 22 16 16)"),
            query2(4, "1981-03-10", R"(Chiang_Kai-shek Napoleon Mohandas_Karamchand_Gandhi Sukarno % component sizes 17 13 11 11)"),
            query2(3, "1982-03-29", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Napoleon % component sizes 13 11 10)"),
            query2(3, "1983-05-09", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Augustine_of_Hippo % component sizes 12 10 8)"),
            query2(5, "1984-07-02", R"(Chiang_Kai-shek Aristotle Mohandas_Karamchand_Gandhi Augustine_of_Hippo Fidel_Castro % component sizes 10 7 6 5 5)"),
            query2(3, "1985-05-31", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5)"),
            query2(3, "1986-06-14", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5)"),
            query2(7, "1987-06-24", R"(Chiang_Kai-shek Augustine_of_Hippo Genghis_Khan Haile_Selassie_I Karl_Marx Lyndon_B._Johnson Robert_John_\"Mutt\"_Lange % component sizes 4 3 3 3 3 3 3)"),

            query3(3, 2, "India"),

            query4(3, "Bill_Clinton"),
// @formatter:on
        };

        return vector;
    }
}
