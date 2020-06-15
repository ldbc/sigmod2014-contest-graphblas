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
                    ((params_stream << query_parameters << ", "), ...);
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
            query1(422, 736, 1,	R"(-1 % path none)"),
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

            query3(3, 2, "Asia", R"(361|812 174|280 280|812 % common interest counts 4 3 3)"),
            query3(4, 3, "Indonesia", R"(396|398 363|367 363|368 363|372 % common interest counts 2 1 1 1)"),
            query3(3, 2, "Egypt", R"(110|116 106|110 106|112 % common interest counts 1 0 0)"),
            query3(3, 2, "Italy", R"(420|825 421|424 10|414 % common interest counts 1 1 0)"),
            query3(5, 4, "Chengdu", R"(590|650 590|658 590|614 590|629 590|638 % common interest counts 1 1 0 0 0)"),
            query3(3, 2, "Peru", R"(65|766 65|767 65|863 % common interest counts 0 0 0)"),
            query3(3, 2, "Democratic_Republic_of_the_Congo", R"(99|100 99|101 99|102 % common interest counts 0 0 0)"),
            query3(7, 6, "Ankara", R"(891|898 890|891 890|895 890|898 890|902 891|895 891|902 % common interest counts 1 0 0 0 0 0 0)"),
            query3(3, 2, "Luoyang", R"(565|625 653|726 565|653 % common interest counts 2 1 0)"),
            query3(4, 3, "Taiwan", R"(795|798 797|798 567|795 567|796 % common interest counts 1 1 0 0)"),
            query3(4, 3, "Brazil", R"(29|31 29|38 29|39 29|59 % common interest counts 1 1 1 1)"),
            query3(9, 8, "Vietnam", R"(404|978 404|979 404|980 404|983 404|984 404|985 404|987 404|990 404|992 % common interest counts 1 1 1 1 1 1 1 1 1)"),
            query3(5, 4, "Australia", R"(8|16 8|17 8|18 8|19 8|163 % common interest counts 0 0 0 0 0)"),

            query4(3, "Bill_Clinton", R"(385 492 819 % centrality values 0.5290135396518375 0.5259615384615384 0.5249520153550864)"),
            query4(4, "Napoleon", R"(722 530 366 316 % centrality values 0.5411255411255411 0.5405405405405406 0.5387931034482758 0.5382131324004306)"),
            query4(3, "Chiang_Kai-shek", R"(592 565 625 % centrality values 0.5453460620525059 0.5421115065243179 0.5408284023668639)"),
            query4(3, "Charles_Darwin", R"(458 305 913 % centrality values 0.5415676959619953 0.5371024734982333 0.5345838218053928)"),
            query4(5, "Ronald_Reagan", R"(953 294 23 100 405 % centrality values 0.5446859903381642 0.5394736842105263 0.5388291517323776 0.5375446960667462 0.5343601895734598)"),
            query4(3, "Aristotle", R"(426 819 429 % centrality values 0.5451219512195121 0.5424757281553397 0.5366146458583433)"),
            query4(3, "George_W._Bush", R"(323 101 541 % centrality values 0.553596806524207 0.5433196380862576 0.5413098243818695)"),
            query4(7, "Tony_Blair", R"(465 647 366 722 194 135 336 % centrality values 0.535629340423861 0.5317101013475888 0.5310624641961301 0.530416402804164 0.5297719114277313 0.5259376153257211 0.5253039555482203)"),
            query4(3, "William_Shakespeare", R"(424 842 23 % centrality values 0.5316276893212858 0.5296265813313688 0.5256692220686188)"),
            query4(4, "Augustine_of_Hippo", R"(385 562 659 323 % centrality values 0.5506329113924051 0.54375 0.54375 0.5291970802919708)"),

// @formatter:on

// Takes extremely long to compile!
//#include "../o1k-queries-cpp.txt"

        };

        return vector;
    }
}
