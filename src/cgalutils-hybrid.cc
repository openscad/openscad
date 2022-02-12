// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"


#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>

#include "CGAL_Nef_polyhedron.h"
#include "polyset-utils.h"

namespace CGALUtils {

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolySet(const PolySet& ps)
{
  auto mesh = make_shared<CGALHybridPolyhedron::mesh_t>();

  auto err = createMeshFromPolySet(ps, *mesh);
  assert(!err);

  triangulateFaces(*mesh);

  if (!ps.is_convex()) {
    if (isClosed(*mesh)) {
      // Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
      // (try testdata/scad/3D/issues/issue1105d.scad for instance), but
      // PMP::orient_to_bound_a_volume seems just fine.
      orientToBoundAVolume(*mesh);
    } else {
      LOG(message_group::Warning, Location::NONE, "", "Warning: mesh is not closed!");
    }
  }

  return make_shared<CGALHybridPolyhedron>(mesh);
}

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromNefPolyhedron(const CGAL_Nef_polyhedron& nef)
{
  assert(nef.p3);

  auto mesh = make_shared<CGALHybridPolyhedron::mesh_t>();

  CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> alien_mesh;
  convertNefPolyhedronToTriangleMesh(*nef.p3, alien_mesh);
  copyMesh(alien_mesh, *mesh);

  return make_shared<CGALHybridPolyhedron>(mesh);
}

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromGeometry(const Geometry& geom)
{
  if (auto poly = dynamic_cast<const CGALHybridPolyhedron *>(&geom)) {
    return make_shared<CGALHybridPolyhedron>(*poly);
  } else if (auto ps = dynamic_cast<const PolySet *>(&geom)) {
    return createHybridPolyhedronFromPolySet(*ps);
  }
  if (auto nef = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
    return createHybridPolyhedronFromNefPolyhedron(*nef);
  } else {
    LOG(message_group::Warning, Location::NONE, "", "Unsupported geometry format.");
    return nullptr;
  }
}

shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron& hybrid)
{
  typedef CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> CGAL_SurfaceMesh;
  if (auto mesh = hybrid.getMesh()) {
    CGAL_SurfaceMesh alien_mesh;
    copyMesh(*mesh, alien_mesh);

    return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
  }
  if (auto nef = hybrid.getNefPolyhedron()) {
    CGALHybridPolyhedron::mesh_t mesh;
    CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, mesh);

    CGAL_SurfaceMesh alien_mesh;
    copyMesh(mesh, alien_mesh);

    return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
  } else {
    assert(!"Invalid hybrid polyhedron state");
    return nullptr;
  }
}

} // namespace CGALUtils

