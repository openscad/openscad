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

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromGeometry(const Geometry &geom)
{
	if (auto poly = dynamic_cast<const CGALHybridPolyhedron *>(&geom)) {
		return make_shared<CGALHybridPolyhedron>(*poly);
	}

#if FAST_CSG_USE_GRID
	const auto use_grid = true;
#else
	const auto use_grid = false;
#endif
	const auto invert_orientation = false;

	if (auto ps = dynamic_cast<const PolySet *>(&geom)) {
		if (!Feature::ExperimentalFastCsgMesh.is_enabled()) {
			SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromGeometry(PolySet -> polyhedron)");
			auto polyhedron = make_shared<CGALHybridPolyhedron::polyhedron_t>();

#ifdef FAST_CSG_LEGACY_TESSELATION
			PolySet ps_tri(3, ps->convexValue());
			ps_tri.setConvexity(ps->getConvexity());
			PolysetUtils::tessellate_faces(*ps, ps_tri);

			auto err = createPolyhedronFromPolySet(ps_tri, *polyhedron, invert_orientation, use_grid);
			assert(!err);
#else
			auto err = createPolyhedronFromPolySet(*ps, *polyhedron, invert_orientation, use_grid);
			assert(!err);

			triangulateFaces(*polyhedron);
#endif // FAST_CSG_LEGACY_TESSELATION

			if (!ps->is_convex()) {
				if (isClosed(*polyhedron)) {
					// Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
					// (try testdata/scad/3D/issues/issue1105d.scad for instance), but
					// PMP::orient_to_bound_a_volume seems just fine.
					orientToBoundAVolume(*polyhedron);
				}
			}

			return make_shared<CGALHybridPolyhedron>(polyhedron);
		}
		else {
			SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromGeometry(PolySet -> mesh)");
			auto mesh = make_shared<CGALHybridPolyhedron::mesh_t>();

#ifdef FAST_CSG_LEGACY_TESSELATION
			PolySet ps_tri(3, ps->convexValue());
			ps_tri.setConvexity(ps->getConvexity());
			PolysetUtils::tessellate_faces(*ps, ps_tri);

			auto err = createMeshFromPolySet(ps_tri, *mesh, use_grid);
			assert(!err);
#else
			auto err = createMeshFromPolySet(*ps, *mesh, use_grid);
			assert(!err);

			triangulateFaces(*mesh);
#endif // FAST_CSG_LEGACY_TESSELATION

			if (!ps->is_convex()) {
				if (isClosed(*mesh)) {
					// Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
					// (try testdata/scad/3D/issues/issue1105d.scad for instance), but
					// PMP::orient_to_bound_a_volume seems just fine.
					orientToBoundAVolume(*mesh);
				}
			}

			return make_shared<CGALHybridPolyhedron>(mesh);
		}
	}
	if (auto nef = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
		assert(nef->p3);

		if (!Feature::ExperimentalFastCsgMesh.is_enabled()) {
			SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromGeometry(nef -> polyhedron)");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
			auto polyhedron = make_shared<CGALHybridPolyhedron::polyhedron_t>();
			CGAL_Polyhedron poly;
			convertNefToPolyhedron(*nef->p3, poly);
			copyPolyhedron(poly, *polyhedron);

			return make_shared<CGALHybridPolyhedron>(polyhedron);
#else
			return make_shared<CGALHybridPolyhedron>(
					make_shared<CGALHybridPolyhedron::nef_polyhedron_t>(*nef->p3));
#endif // FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		}
		else {
			SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromGeometry(nef -> mesh)");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
			auto mesh = make_shared<CGALHybridPolyhedron::mesh_t>();

			CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> alien_mesh;
			convertNefPolyhedronToMesh(*nef->p3, alien_mesh);
			copyMesh(alien_mesh, *mesh);

			return make_shared<CGALHybridPolyhedron>(mesh);
#else
			return make_shared<CGALHybridPolyhedron>(
					make_shared<CGALHybridPolyhedron::nef_polyhedron_t>(*nef->p3));
#endif // FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		}
	}
	else {
		LOG(message_group::Warning, Location::NONE, "", "Unsupported geometry format.");
		return nullptr;
	}
}

shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron &hybrid)
{
	if (auto poly = hybrid.getPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("createNefPolyhedronFromHybrid: polyhedron -> nef");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		CGAL_Polyhedron alien_poly;
		copyPolyhedron(*poly, alien_poly);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
#else
		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(*poly));
#endif
	}
	if (auto mesh = hybrid.getMesh()) {
		SCOPED_PERFORMANCE_TIMER("createNefPolyhedronFromHybrid: mesh -> nef");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> alien_mesh;
		copyMesh(*mesh, alien_mesh);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_mesh));
#else
		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(*mesh));
#endif
	}
	if (auto nef = hybrid.getNefPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("createNefPolyhedronFromHybrid: nef -> nef");

#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
		CGALHybridPolyhedron::polyhedron_t poly;
		convertNefToPolyhedron(*nef, poly);

		CGAL_Polyhedron alien_poly;
		copyPolyhedron(poly, alien_poly);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
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
