#pragma once

#include <string>
#include <tuple>

struct BaseQuery {
    virtual std::tuple<std::string, std::string> initial() = 0;
};
