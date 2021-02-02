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
	std::size_t hash<Geometry::GeometryItem>::operator()(Geometry::GeometryItem const& s) const {
    size_t seed = 0;
    boost::hash_combine(seed, s.first);
    boost::hash_combine(seed, s.second);
    return seed;
  }
	std::size_t hash<CGAL::Point_3<CGAL::Epeck>>::operator()(const CGAL::Point_3<CGAL::Epeck> &s) const {
    hash<Vector3d> vh;
    // Very slow?
    return vh(Vector3d(CGAL::to_double(s.x()), CGAL::to_double(s.y()), CGAL::to_double(s.z())));
	}
}

namespace Eigen {
  size_t hash_value(Vector3f const &v) {
    size_t seed = 0;
    for (int i=0; i<3; ++i) boost::hash_combine(seed, v[i]);
    return seed;
  }
  size_t hash_value(Vector3d const &v) {
    size_t seed = 0;
    for (int i=0; i<3; ++i) boost::hash_combine(seed, v[i]);
    return seed;
  }
  size_t hash_value(Eigen::Matrix<int64_t, 3, 1> const &v) {
    size_t seed = 0;
    for (int i=0; i<3; ++i) boost::hash_combine(seed, v[i]);
    return seed;
  }
}
