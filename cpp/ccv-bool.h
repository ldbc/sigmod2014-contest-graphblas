#pragma once

#include "gb_utils.h"

std::tuple<GBxx_Object<GrB_Vector>, std::unique_ptr<GrB_Index[]>> compute_ccv_bool(GrB_Matrix A);
