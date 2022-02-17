#pragma once

#include "cgal.h"
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"
#include "enums.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
typedef CGAL::Epick K;
typedef CGAL::Point_3<K> Vertex3K;
typedef std::vector<Vertex3K> PolygonK;
typedef std::vector<PolygonK> PolyholeK;

class CGALHybridPolyhedron;

namespace CGAL {
template <typename P>
class Surface_mesh;
}

namespace CGAL {
inline std::size_t hash_value(const CGAL_HybridKernel3::FT& x) {
  std::hash<double> dh;
  return dh(CGAL::to_double(x));
}
}

namespace /* anonymous */ {
template <typename Result, typename V>
Result vector_convert(V const& v) {
  return Result(CGAL::to_double(v[0]), CGAL::to_double(v[1]), CGAL::to_double(v[2]));
}
}

namespace CGALUtils {

bool applyHull(const Geometry::Geometries& children, PolySet& P);
template <typename K>
bool is_weakly_convex(const CGAL::Polyhedron_3<K>& p);
template <typename K>
bool is_weakly_convex(const CGAL::Surface_mesh<CGAL::Point_3<K>>& m);
shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries& children, OpenSCADOperator op);
shared_ptr<const Geometry> applyUnion3D(Geometry::Geometries::iterator chbegin, Geometry::Geometries::iterator chend);
shared_ptr<CGALHybridPolyhedron> applyOperator3DHybrid(const Geometry::Geometries& children, OpenSCADOperator op);
shared_ptr<CGALHybridPolyhedron> applyUnion3DHybrid(
  const Geometry::Geometries::const_iterator& chbegin,
  const Geometry::Geometries::const_iterator& chend);
//FIXME: Old, can be removed:
//void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
Polygon2d *project(const CGAL_Nef_polyhedron& N, bool cut);
template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Nef_polyhedron_3<K>& N);
template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh);
CGAL_Iso_cuboid_3 boundingBox(const Geometry& geom);
CGAL_Iso_cuboid_3 createIsoCuboidFromBoundingBox(const BoundingBox& bbox);
bool is_approximately_convex(const PolySet& ps);
shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
shared_ptr<const Geometry> applyMinkowskiHybrid(const Geometry::Geometries& children);

template <typename Polyhedron> bool createPolySetFromPolyhedron(const Polyhedron& p, PolySet& ps);
template <class InputKernel, class OutputKernel>
void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel>& poly_a, CGAL::Polyhedron_3<OutputKernel>& poly_b);
template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet& ps, Polyhedron& p);

template <typename K>
bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, PolySet& ps);
template <class InputKernel, class OutputKernel>
void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
              CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>& output);
template <typename K>
bool createMeshFromPolySet(const PolySet& ps, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh);

template <typename K>
bool createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<K>& N, PolySet& ps);
shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromGeometry(const class Geometry &geom);
shared_ptr<const PolySet> getGeometryAsPolySet(const shared_ptr<const Geometry>&);
shared_ptr<const CGAL_Nef_polyhedron> getGeometryAsNefPolyhedron(const shared_ptr<const Geometry>&);

template <typename K>
CGAL::Aff_transformation_3<K> createAffineTransformFromMatrix(const Transform3d& matrix);
template <typename K>
void transform(CGAL::Nef_polyhedron_3<K>& N, const Transform3d& matrix);
template <typename K>
void transform(CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, const Transform3d& matrix);
template <typename K>
Transform3d computeResizeTransform(
  const CGAL::Iso_cuboid_3<K>& bb, int dimension, const Vector3d& newsize,
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
shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron& hybrid);
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
template <typename K>
bool corefineAndComputeUnion(CGAL::Polyhedron_3<K>& lhs, CGAL::Polyhedron_3<K>& rhs, CGAL::Polyhedron_3<K>& out);
template <typename K>
bool corefineAndComputeIntersection(CGAL::Polyhedron_3<K>& lhs, CGAL::Polyhedron_3<K>& rhs, CGAL::Polyhedron_3<K>& out);
template <typename K>
bool corefineAndComputeDifference(CGAL::Polyhedron_3<K>& lhs, CGAL::Polyhedron_3<K>& rhs, CGAL::Polyhedron_3<K>& out);
template <typename K>
bool corefineAndComputeUnion(CGAL::Surface_mesh<CGAL::Point_3<K>>& lhs, CGAL::Surface_mesh<CGAL::Point_3<K>>& rhs, CGAL::Surface_mesh<CGAL::Point_3<K>>& out);
template <typename K>
bool corefineAndComputeIntersection(CGAL::Surface_mesh<CGAL::Point_3<K>>& lhs, CGAL::Surface_mesh<CGAL::Point_3<K>>& rhs, CGAL::Surface_mesh<CGAL::Point_3<K>>& out);
template <typename K>
bool corefineAndComputeDifference(CGAL::Surface_mesh<CGAL::Point_3<K>>& lhs, CGAL::Surface_mesh<CGAL::Point_3<K>>& rhs, CGAL::Surface_mesh<CGAL::Point_3<K>>& out);

template <typename K>
void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh);
void cleanupMesh(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh, bool is_corefinement_result);

std::shared_ptr<const CGAL_HybridMesh> getMeshFromNDGeometry(const std::shared_ptr<const Geometry>& geom);

template <typename TriangleMesh, typename OutStream>
void dumpMesh(const TriangleMesh& tm, size_t dimension, size_t convexity, OutStream& out, const std::string& indent, size_t currindent);
} // namespace CGALUtils
