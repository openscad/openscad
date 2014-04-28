#pragma once

#include <boost/unordered_map.hpp>
#include "value.h"

extern boost::unordered_map<std::string,Value> dxf_dim_cache;
extern boost::unordered_map<std::string,Value> dxf_cross_cache;
