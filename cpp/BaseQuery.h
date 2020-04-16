#pragma once

#include <chrono>
#include <utility>
#include "load.h"

struct BaseQuery {
    virtual void load() = 0;

    virtual void initial() = 0;

    virtual void update(int iteration) = 0;
};
