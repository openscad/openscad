#pragma once

#include "geometry/cgal/cgal.h"
#include "geometry/PolySet.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include "core/enums.h"

#include <memory>
#include <cstddef>
#include <vector>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

using K = CGAL::Epick;
using Vertex3K = CGAL::Point_3<K>;
using PolygonK = std::vector<Vertex3K>;
using PolyholeK = std::vector<PolygonK>;

class CGALHybridPolyhedron;

namespace CGAL {
inline std::size_t hash_value(const CGAL_HybridKernel3::FT& x) {
  std::hash<double> dh;
  return dh(CGAL::to_double(x));
}
}

namespace CGALUtils {

template <typename Result, typename V>
Result vector_convert(V const& v) {
  return Result(CGAL::to_double(v[0]), CGAL::to_double(v[1]), CGAL::to_double(v[2]));
}

std::unique_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromPolySet(const PolySet& ps);
template <typename K>
bool is_weakly_convex(const CGAL::Polyhedron_3<K>& p);
template <typename K>
bool is_weakly_convex(const CGAL::Surface_mesh<CGAL::Point_3<K>>& m);
std::shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries& children, OpenSCADOperator op);
std::unique_ptr<const Geometry> applyUnion3D(Geometry::Geometries::iterator chbegin, Geometry::Geometries::iterator chend);
std::shared_ptr<CGALHybridPolyhedron> applyOperator3DHybrid(const Geometry::Geometries& children, OpenSCADOperator op);
std::shared_ptr<CGALHybridPolyhedron> applyUnion3DHybrid(
  const Geometry::Geometries::const_iterator& chbegin,
  const Geometry::Geometries::const_iterator& chend);
//FIXME: Old, can be removed:
//void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
std::unique_ptr<Polygon2d> project(const CGAL_Nef_polyhedron& N, bool cut);
template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Nef_polyhedron_3<K>& N);
template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh);
CGAL_Iso_cuboid_3 createIsoCuboidFromBoundingBox(const BoundingBox& bbox);
bool is_approximately_convex(const PolySet& ps);
std::shared_ptr<const Geometry> applyMinkowskiHybrid(const Geometry::Geometries& children);

template <typename Polyhedron> std::unique_ptr<PolySet> createPolySetFromPolyhedron(const Polyhedron& p);
template <class InputKernel, class OutputKernel>
void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel>& poly_a, CGAL::Polyhedron_3<OutputKernel>& poly_b);
template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet& ps, Polyhedron& p);

template <class TriangleMesh>
std::unique_ptr<PolySet> createPolySetFromMesh(const TriangleMesh& mesh);
template <class InputKernel, class OutputKernel>
void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
              CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>& output);
template <class TriangleMesh>
bool createMeshFromPolySet(const PolySet& ps, TriangleMesh& mesh);

template <typename K>
std::unique_ptr<PolySet> createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<K>& N);
std::shared_ptr<const CGAL_Nef_polyhedron> getNefPolyhedronFromGeometry(const std::shared_ptr<const Geometry>& geom);
std::shared_ptr<const CGAL_Nef_polyhedron> getGeometryAsNefPolyhedron(const std::shared_ptr<const Geometry>&);

template <typename K>
CGAL::Aff_transformation_3<K> createAffineTransformFromMatrix(const Transform3d& matrix);
template <typename K>
void transform(CGAL::Nef_polyhedron_3<K>& N, const Transform3d& matrix);
template <typename K>
void transform(CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, const Transform3d& matrix);
template <typename K>
Transform3d computeResizeTransform(
  const CGAL::Iso_cuboid_3<K>& bb, unsigned int dimension, const Vector3d& newsize,
  const Eigen::Matrix<bool, 3, 1>& autosize);
bool tessellatePolygon(const PolygonK& polygon,
                       Polygons& triangles,
                       const K::Vector_3 *normal = nullptr);
bool tessellatePolygonWithHoles(const PolyholeK& polygons,
                                Polygons& triangles,
                                const K::Vector_3 *normal = nullptr);
bool tessellate3DFaceWithHoles(std::vector<CGAL_Polygon_3>& polygons,
                               std::vector<CGAL_Polygon_3>& triangles,
                               CGAL::Plane_3<CGAL_Kernel3>& plane);
template <typename FromKernel, typename ToKernel>
struct KernelConverter {
  // Note: we could have this return `CGAL::to_double(n)` by default, but
  // that would mean that failure to provide a proper specialization would
  // default to lossy conversion.
  typename ToKernel::FT operator()(const typename FromKernel::FT& n) const;
};
template <typename FromKernel, typename ToKernel>
CGAL::Cartesian_converter<FromKernel, ToKernel, KernelConverter<FromKernel, ToKernel>>
getCartesianConverter()
{
  return CGAL::Cartesian_converter<
    FromKernel, ToKernel, KernelConverter<FromKernel, ToKernel>>();
}
std::shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron& hybrid);
std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolySet(const PolySet& ps);
std::shared_ptr<CGALHybridPolyhedron> createMutableHybridPolyhedronFromGeometry(const std::shared_ptr<const Geometry>& geom);
std::shared_ptr<const CGALHybridPolyhedron> getHybridPolyhedronFromGeometry(const std::shared_ptr<const Geometry>& geom);
template <typename K>
std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolyhedron(const CGAL::Polyhedron_3<K>& poly);
template <typename Polyhedron>
void triangulateFaces(Polyhedron& polyhedron);
template <typename Polyhedron>
bool isClosed(const Polyhedron& polyhedron);
template <typename Polyhedron>
void orientToBoundAVolume(Polyhedron& polyhedron);
template <typename Polyhedron>
void reverseFaceOrientations(Polyhedron& polyhedron);
template <typename K>
void inPlaceNefUnion(CGAL::Nef_polyhedron_3<K>& lhs, const CGAL::Nef_polyhedron_3<K>& rhs);
template <typename K>
void inPlaceNefDifference(CGAL::Nef_polyhedron_3<K>& lhs, const CGAL::Nef_polyhedron_3<K>& rhs);
template <typename K>
void inPlaceNefIntersection(CGAL::Nef_polyhedron_3<K>& lhs, const CGAL::Nef_polyhedron_3<K>& rhs);
template <typename K>
void inPlaceNefMinkowski(CGAL::Nef_polyhedron_3<K>& lhs, CGAL::Nef_polyhedron_3<K>& rhs);
template <typename K>
void convertNefToPolyhedron(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Polyhedron_3<K>& polyhedron);
template <class TriangleMesh>
bool corefineAndComputeUnion(TriangleMesh& lhs, TriangleMesh& rhs, TriangleMesh& out);
template <class TriangleMesh>
bool corefineAndComputeIntersection(TriangleMesh& lhs, TriangleMesh& rhs, TriangleMesh& out);
template <class TriangleMesh>
bool corefineAndComputeDifference(TriangleMesh& lhs, TriangleMesh& rhs, TriangleMesh& out);

template <typename K>
void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh);
void cleanupMesh(CGAL_HybridMesh& mesh, bool is_corefinement_result);

std::unique_ptr<PolySet> createTriangulatedPolySetFromPolygon2d(const Polygon2d& polygon2d);

} // namespace CGALUtils
