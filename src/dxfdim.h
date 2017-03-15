#pragma once

#include <unordered_map>
#include "value.h"

extern std::unordered_map<std::string, ValuePtr> dxf_dim_cache;
extern std::unordered_map<std::string, ValuePtr> dxf_cross_cache;
