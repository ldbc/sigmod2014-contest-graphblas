#pragma once

#include <chrono>
#include <utility>
#include "load.h"

struct BaseQuery {
    virtual std::tuple<std::string, std::string> initial() = 0;
};
