#pragma once

#include <functional>
#include "gb_utils.h"

std::tuple<GBxx_Object<GrB_Vector>, std::unique_ptr<GrB_Index[]>>
compute_ccv(GrB_Matrix A, std::function<void(std::string, std::string)> const &add_comment);
