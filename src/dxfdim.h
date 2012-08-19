#ifndef DXFDIM_H_
#define DXFDIM_H_

//#include <boost/unordered_map.hpp>
#include <map>
#include "value.h"

//extern boost::unordered_map<std::string,Value> dxf_dim_cache;
//extern boost::unordered_map<std::string,Value> dxf_cross_cache;
extern std::map<std::string,Value> dxf_dim_cache;
extern std::map<std::string,Value> dxf_cross_cache;

#endif
