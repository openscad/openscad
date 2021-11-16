#include "hash.h"
#include <boost/functional/hash.hpp>

namespace std {
	std::size_t hash<Vector3f>::operator()(const Vector3f &s) const {
		return Eigen::hash_value(s);
	}
	std::size_t hash<Vector3d>::operator()(const Vector3d &s) const {
		return Eigen::hash_value(s);
	}
	std::size_t hash<Vector3l>::operator()(const Vector3l &s) const {
		return Eigen::hash_value(s);
	}
}

namespace Eigen {
  size_t hash_value(Vector3f const &v) {
    size_t seed = 0;
#if 1
    uint32_t temp[3];
    ((float *)temp)[0] = v[0];   // recast float -> uint32_t for faster hashing
    ((float *)temp)[1] = v[1];
    ((float *)temp)[2] = v[2];
    seed  = (size_t)temp[0] + 0x9e3779b9;   // inline hash combine calculations
    seed ^= (size_t)temp[1] + 0x9e3779b9 + (seed<<6) + (seed>>2);
    seed ^= (size_t)temp[2] + 0x9e3779b9 + (seed<<6) + (seed>>2);
#else
    for (int i=0; i<3; ++i) boost::hash_combine(seed, v[i]);
#endif
    return seed;
  }
  size_t hash_value(Vector3d const &v) {
    size_t seed = 0;
#if 1
    uint64_t temp[3];
    ((double *)temp)[0] = v[0];   // recast double -> uint64_t for faster hashing
    ((double *)temp)[1] = v[1];
    ((double *)temp)[2] = v[2];
    seed  = (size_t)temp[0] + 0x9e3779b9;   // inline hash combine calculations
    seed ^= (size_t)temp[1] + 0x9e3779b9 + (seed<<6) + (seed>>2);
    seed ^= (size_t)temp[2] + 0x9e3779b9 + (seed<<6) + (seed>>2);
#else
    for (int i=0; i<3; ++i) boost::hash_combine(seed, v[i]);
#endif
    return seed;
  }
  size_t hash_value(Eigen::Matrix<int64_t, 3, 1> const &v) {
    size_t seed = 0;
    for (int i=0; i<3; ++i) boost::hash_combine(seed, v[i]);
    return seed;
  }
}
