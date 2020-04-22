#include "query-parameters.h"
#include "Query1.h"
#include "Query2.h"
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

    std::vector<std::function<std::string()>> vector{
// formatter markers: https://stackoverflow.com/a/19492318
// @formatter:off
            query1(58, 402, 0),

            query2(3, "1980-02-01", R"(Chiang_Kai-shek Augustine_of_Hippo Napoleon % component sizes 22 16 16)"),
            query2(4, "1981-03-10", R"(Chiang_Kai-shek Napoleon Mohandas_Karamchand_Gandhi Sukarno % component sizes 17 13 11 11)"),
            query2(3, "1982-03-29", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Napoleon % component sizes 13 11 10)"),
            query2(3, "1983-05-09", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Augustine_of_Hippo % component sizes 12 10 8)"),
            query2(5, "1984-07-02", R"(Chiang_Kai-shek Aristotle Mohandas_Karamchand_Gandhi Augustine_of_Hippo Fidel_Castro % component sizes 10 7 6 5 5)"),
            query2(3, "1985-05-31", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5)"),
            query2(3, "1986-06-14", R"(Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5)"),
            query2(7, "1987-06-24", R"(Chiang_Kai-shek Augustine_of_Hippo Genghis_Khan Haile_Selassie_I Karl_Marx Lyndon_B._Johnson Robert_John_\"Mutt\"_Lange % component sizes 4 3 3 3 3 3 3)"),
// @formatter:on
    };

    return vector;
}
