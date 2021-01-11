#pragma once

#include <string>
#include <tuple>
#include <chrono>
#include <utility>
#include <memory>

struct BaseQuery {
    virtual std::tuple<std::string, std::string> initial() = 0;

    virtual int getQueryId() const = 0;
};
