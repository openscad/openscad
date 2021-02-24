// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>

#include "CGAL_Nef_polyhedron.h"
#include "polyset-utils.h"
#include "scoped_timer.h"

namespace CGALUtils {

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromPolySet(const PolySet &ps)
{
  SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromPolySet");
  auto mesh = make_shared<CGALHybridPolyhedron::mesh_t>();

  auto err = createMeshFromPolySet(ps, *mesh, /* use_grid */ false);
  assert(!err);

  triangulateFaces(*mesh);

  if (!ps.is_convex()) {
    if (isClosed(*mesh)) {
      // Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
      // (try testdata/scad/3D/issues/issue1105d.scad for instance), but
      // PMP::orient_to_bound_a_volume seems just fine.
      orientToBoundAVolume(*mesh);
    }
  }

  return make_shared<CGALHybridPolyhedron>(mesh);
}

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromNefPolyhedron(const CGAL_Nef_polyhedron &nef)
{
  assert(nef.p3);

  SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromNefPolyhedron");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
  auto mesh = make_shared<CGALHybridPolyhedron::mesh_t>();

  CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> alien_mesh;
  convertNefPolyhedronToTriangleMesh(*nef.p3, alien_mesh);
  copyMesh(alien_mesh, *mesh);

  return make_shared<CGALHybridPolyhedron>(mesh);
#else
  return make_shared<CGALHybridPolyhedron>(
      make_shared<CGALHybridPolyhedron::nef_polyhedron_t>(*nef.p3));
#endif // FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
}

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromGeometry(const Geometry &geom)
{
	if (auto poly = dynamic_cast<const CGALHybridPolyhedron *>(&geom)) {
		return make_shared<CGALHybridPolyhedron>(*poly);
	}
	else if (auto ps = dynamic_cast<const PolySet *>(&geom)) {
		return createHybridPolyhedronFromPolySet(*ps);
	}
	if (auto nef = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
		return createHybridPolyhedronFromNefPolyhedron(*nef);
	}
	else {
		LOG(message_group::Warning, Location::NONE, "", "Unsupported geometry format.");
		return nullptr;
	}
}

shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron &hybrid)
{
	typedef CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> CGAL_SurfaceMesh;
	if (auto mesh = hybrid.getMesh()) {
		SCOPED_PERFORMANCE_TIMER("createNefPolyhedronFromHybrid: mesh -> nef");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		CGAL_SurfaceMesh alien_mesh;
		copyMesh(*mesh, alien_mesh);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
#else
		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(*mesh));
#endif
	}
	if (auto nef = hybrid.getNefPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("createNefPolyhedronFromHybrid: nef -> nef");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		CGALHybridPolyhedron::mesh_t mesh;
		CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, mesh);

		CGAL_SurfaceMesh alien_mesh;
    copyMesh(mesh, alien_mesh);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
#else
		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(*nef));
#endif // FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
	}
	else {
		assert(!"Invalid hybrid polyhedron state");
		return nullptr;
	}
}

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE
