#pragma once

#include <vector>
#include <functional>
#include <string>
#include "utils.h"
#include "input.h"

std::vector<std::function<std::string(BenchmarkParameters const &, QueryInput const &)>>
getQueriesWithParameters(BenchmarkParameters &benchmark_parameters);
