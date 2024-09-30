// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"


#include <cassert>
#include <memory>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/convex_hull_3.h>
#include <cstddef>

#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include "geometry/PolySetUtils.h"
#if ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include <vector>

namespace CGALUtils {

template <typename K>
std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolyhedron(const CGAL::Polyhedron_3<K>& poly)
{
  CGAL::Surface_mesh<CGAL::Point_3<K>> mesh;
  CGAL::copy_face_graph(poly, mesh);

  auto hybrid_mesh = std::make_shared<CGAL_HybridMesh>();
  copyMesh(mesh, *hybrid_mesh);
  CGALUtils::triangulateFaces(*hybrid_mesh);

  return std::make_shared<CGALHybridPolyhedron>(hybrid_mesh);
}

template std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Epick>& poly);

bool hasOnlyTriangles(const PolySet& ps) {
  for (auto& p : ps.indices) {
    if (p.size() != 3) {
      return false;
    }
  }
  return true;
}

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolySet(const PolySet& ps)
{
  // Since is_convex doesn't work well with non-planar faces,
  // we tessellate the polyset before checking.
  PolySet psq(ps);
  std::vector<Vector3d> points3d;
  psq.quantizeVertices(&points3d);
  auto ps_tri = PolySetUtils::tessellate_faces(psq);
  if (ps_tri->isConvex()) {
    using K = CGAL::Epick;
    // Collect point cloud
    std::vector<K::Point_3> points(points3d.size());
    for (size_t i = 0, n = points3d.size(); i < n; i++) {
      points[i] = vector_convert<K::Point_3>(points3d[i]);
    }
    if (points.size() <= 3) return std::make_shared<CGALHybridPolyhedron>();

    // Apply hull
    CGAL::Surface_mesh<CGAL::Point_3<K>> r;
    CGAL::convex_hull_3(points.begin(), points.end(), r);
    auto r_exact = std::make_shared<CGAL_HybridMesh>();
    copyMesh(r, *r_exact);
    return std::make_shared<CGALHybridPolyhedron>(r_exact);
  }

  auto mesh = std::make_shared<CGAL_HybridMesh>();
  if (createMeshFromPolySet(*ps_tri, *mesh)) {
    assert(false && "Error from createMeshFromPolySet");
  }
  if (!ps_tri->isConvex()) {
    if (isClosed(*mesh)) {
      // Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
      // (try testdata/scad/3D/issues/issue1105d.scad for instance), but
      // PMP::orient_to_bound_a_volume seems just fine.
      orientToBoundAVolume(*mesh);
    } else {
      LOG(message_group::Warning, "Warning: mesh is not closed!");
    }
  }

  return std::make_shared<CGALHybridPolyhedron>(mesh);
}

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromNefPolyhedron(const CGAL_Nef_polyhedron& nef)
{
  assert(nef.p3);

  auto mesh = std::make_shared<CGAL_HybridMesh>();

  CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> alien_mesh;
  convertNefPolyhedronToTriangleMesh(*nef.p3, alien_mesh);
  copyMesh(alien_mesh, *mesh);

  return std::make_shared<CGALHybridPolyhedron>(mesh);
}

std::shared_ptr<CGALHybridPolyhedron> createMutableHybridPolyhedronFromGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (auto poly = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return std::make_shared<CGALHybridPolyhedron>(*poly);
  } else if (auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return createHybridPolyhedronFromPolySet(*ps);
  } else if (auto nef = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    return createHybridPolyhedronFromNefPolyhedron(*nef);
#if ENABLE_MANIFOLD
  } else if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return createHybridPolyhedronFromPolySet(*mani->toPolySet());
#endif
  } else {
    LOG(message_group::Warning, "Unsupported geometry format.");
    return nullptr;
  }
}

std::shared_ptr<const CGALHybridPolyhedron> getHybridPolyhedronFromGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (auto poly = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return poly;
  } else {
    return createMutableHybridPolyhedronFromGeometry(geom);
  }
}

std::shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron& hybrid)
{
  using CGAL_SurfaceMesh = CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>>;
  if (auto mesh = hybrid.getMesh()) {
    CGAL_SurfaceMesh alien_mesh;
    copyMesh(*mesh, alien_mesh);

    return std::make_shared<CGAL_Nef_polyhedron>(std::make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
  }
  if (auto nef = hybrid.getNefPolyhedron()) {
    CGAL_HybridMesh mesh;
    CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, mesh);

    CGAL_SurfaceMesh alien_mesh;
    copyMesh(mesh, alien_mesh);

    return std::make_shared<CGAL_Nef_polyhedron>(std::make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
  } else {
    assert(!"Invalid hybrid polyhedron state");
    return nullptr;
  }
}

} // namespace CGALUtils
