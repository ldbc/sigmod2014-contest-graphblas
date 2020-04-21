#pragma once

#include <chrono>
#include <utility>
#include "load.h"

struct BaseQuery {
    virtual void initial() = 0;
};
