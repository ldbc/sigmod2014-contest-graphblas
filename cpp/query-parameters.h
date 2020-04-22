#pragma once

#include <vector>
#include <functional>
#include <string>
#include "utils.h"
#include "load.h"

std::vector<std::function<std::string()>>
getQueriesWithParameters(BenchmarkParameters benchmark_parameters, QueryInput const &input);
