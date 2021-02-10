#pragma once

#include "linalg.h"
#include "Geometry.h"

#ifdef ENABLE_CGAL
#include "cgal.h"
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#endif

typedef Eigen::Matrix<int64_t, 3, 1> Vector3l;

namespace std {
	template<> struct hash<Vector3f> { std::size_t operator()(const Vector3f &s) const; };
	template<> struct hash<Vector3d> { std::size_t operator()(const Vector3d &s) const; };
	template<> struct hash<Vector3l> { std::size_t operator()(const Vector3l &s) const; };
	template<> struct hash<Geometry::GeometryItem> { std::size_t operator()(const Geometry::GeometryItem &s) const; };

#ifdef ENABLE_CGAL
  template<> struct hash<CGAL::Point_3<CGAL_Kernel3>> { std::size_t operator()(const CGAL::Point_3<CGAL_Kernel3> &s) const; };
  template<> struct hash<CGAL::Point_3<CGAL::Epeck>> { std::size_t operator()(const CGAL::Point_3<CGAL::Epeck> &s) const; };
#endif
}

namespace Eigen {
	size_t hash_value(Vector3f const &v);
	size_t hash_value(Vector3d const &v);
	size_t hash_value(Vector3l const &v);
}
