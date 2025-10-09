#pragma once

#include <memory>
#include <vector>

#ifdef ENABLE_CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include "geometry/cgal/CGALNefGeometry.h"
#include "geometry/cgal/cgal.h"
#endif

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"
#include "core/enums.h"

namespace CGALUtils {

#ifdef ENABLE_CGAL
template <typename Result, typename V>
Result vector_convert(V const& v)
{
  return Result(CGAL::to_double(v[0]), CGAL::to_double(v[1]), CGAL::to_double(v[2]));
}

std::unique_ptr<CGALNefGeometry> createNefPolyhedronFromPolySet(const PolySet& ps);
template <typename K>
bool is_weakly_convex(const CGAL::Polyhedron_3<K>& p);

template <typename K>
bool is_weakly_convex(const CGAL::Surface_mesh<CGAL::Point_3<K>>& m);

std::shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries& children,
                                                OpenSCADOperator op);
std::unique_ptr<const Geometry> applyUnion3D(Geometry::Geometries::iterator chbegin,
                                             Geometry::Geometries::iterator chend);
std::shared_ptr<const Geometry> applyMinkowski3D(const Geometry::Geometries& children);

std::unique_ptr<Polygon2d> project(const CGALNefGeometry& N, bool cut);
template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Nef_polyhedron_3<K>& N);

template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh);

CGAL_Iso_cuboid_3 createIsoCuboidFromBoundingBox(const BoundingBox& bbox);
bool is_approximately_convex(const PolySet& ps);

template <typename Polyhedron>
std::unique_ptr<PolySet> createPolySetFromPolyhedron(const Polyhedron& p);

template <typename Polyhedron>
bool createPolyhedronFromPolySet(const PolySet& ps, Polyhedron& p);

template <class InputKernel, class OutputKernel>
void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel>& poly_a,
                    CGAL::Polyhedron_3<OutputKernel>& poly_b);

template <class InputKernel, class OutputKernel>
void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
              CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>& output);

CGAL_DoubleMesh repairPolySet(const PolySet& ps);

template <class SurfaceMesh>
std::shared_ptr<SurfaceMesh> createSurfaceMeshFromPolySet(const PolySet& ps);
template <class SurfaceMesh>
std::unique_ptr<PolySet> createPolySetFromSurfaceMesh(const SurfaceMesh& mesh);

std::unique_ptr<PolySet> createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3& N);

std::shared_ptr<const CGALNefGeometry> getNefPolyhedronFromGeometry(
  const std::shared_ptr<const Geometry>& geom);
std::shared_ptr<const CGALNefGeometry> getGeometryAsNefPolyhedron(
  const std::shared_ptr<const Geometry>&);

template <typename K>
CGAL::Aff_transformation_3<K> createAffineTransformFromMatrix(const Transform3d& matrix);
template <typename K>
void transform(CGAL::Nef_polyhedron_3<K>& N, const Transform3d& matrix);
template <typename K>
void transform(CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, const Transform3d& matrix);
template <typename K>
Transform3d computeResizeTransform(const CGAL::Iso_cuboid_3<K>& bb, unsigned int dimension,
                                   const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize);
bool tessellatePolygon(const std::vector<CGAL::Point_3<CGAL::Epick>>& polygon, Polygons& triangles,
                       const CGAL::Vector_3<CGAL::Epick> *normal = nullptr);
bool tessellatePolygonWithHoles(const std::vector<std::vector<CGAL::Point_3<CGAL::Epick>>>& polygons,
                                Polygons& triangles,
                                const CGAL::Vector_3<CGAL::Epick> *normal = nullptr);
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
  return CGAL::Cartesian_converter<FromKernel, ToKernel, KernelConverter<FromKernel, ToKernel>>();
}

template <typename SurfaceMesh>
void triangulateFaces(SurfaceMesh& mesh);
template <typename Polyhedron>
bool isClosed(const Polyhedron& polyhedron);
template <typename SurfaceMesh>
void orientToBoundAVolume(SurfaceMesh& mesh);
template <typename K>
void convertNefToPolyhedron(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Polyhedron_3<K>& polyhedron);

void convertNefToSurfaceMesh(const CGAL_Nef_polyhedron3& nef, CGAL_Kernel3Mesh& mesh);
template <typename SurfaceMesh>
CGAL_Nef_polyhedron3 convertSurfaceMeshToNef(const SurfaceMesh& mesh);

#endif
std::unique_ptr<PolySet> createTriangulatedPolySetFromPolygon2d(const Polygon2d& polygon2d);

}  // namespace CGALUtils
