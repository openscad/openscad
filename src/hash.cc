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
	for (int i = 0; i < 3; i++) boost::hash_combine(seed, v[i]);
	return seed;
}
size_t hash_value(Vector3d const &v) {
	size_t seed = 0;
	for (int i = 0; i < 3; i++) boost::hash_combine(seed, v[i]);
	return seed;
}
size_t hash_value(Eigen::Matrix<int64_t, 3, 1> const &v) {
	size_t seed = 0;
	for (int i = 0; i < 3; i++) boost::hash_combine(seed, v[i]);
	return seed;
}
}
