// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#if CGAL_VERSION_NR >= CGAL_VERSION_NUMBER(5, 1, 0)
#include <CGAL/Polygon_mesh_processing/manifoldness.h>
#else
#include <CGAL/Polygon_mesh_processing/repair.h>
#endif

#include "CGAL_Nef_polyhedron.h"
#include "polyset.h"
#include "scoped_timer.h"

namespace PMP = CGAL::Polygon_mesh_processing;

namespace CGALUtils {

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromGeometry(const Geometry &geom)
{
	if (auto poly = dynamic_cast<const CGALHybridPolyhedron *>(&geom)) {
		return make_shared<CGALHybridPolyhedron>(*poly);
	}
	else if (auto ps = dynamic_cast<const PolySet *>(&geom)) {
		SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFromGeometry(PolySet)");

		auto polyhedron = make_shared<CGALHybridPolyhedron::polyhedron_t>();

		auto err =
				CGALUtils::createPolyhedronFromPolySet(*ps, *polyhedron, /* invert_orientation */ false);
		assert(!err);

		PMP::triangulate_faces(*polyhedron);

		if (!ps->is_manifold() && !ps->is_convex()) {
			if (CGAL::is_closed(*polyhedron)) {
				// Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
				// (try testdata/scad/3D/issues/issue1105d.scad for instance), but
				// PMP::orient_to_bound_a_volume seems just fine.
				PMP::orient_to_bound_a_volume(*polyhedron);
			}
		}

		return make_shared<CGALHybridPolyhedron>(polyhedron);
	}
	else if (auto nef = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
		assert(nef->p3);

		SCOPED_PERFORMANCE_TIMER("createHybridPolyhedronFomGeometry(CGAL_Nef_polyhedron)");

		auto polyhedron = make_shared<CGALHybridPolyhedron::polyhedron_t>();
		CGAL_Polyhedron poly;
		nef->p3->convert_to_polyhedron(poly);
		CGALUtils::copyPolyhedron(poly, *polyhedron);

		return make_shared<CGALHybridPolyhedron>(polyhedron);
	}
	else {
		LOG(message_group::Warning, Location::NONE, "", "Unsupported geometry format.");
		return nullptr;
	}
}

shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron &hybrid)
{
	if (auto poly = hybrid.getPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("createNefPolyhedronFromHybrid(Polyhedron_3)");

		CGAL_Polyhedron alien_poly;
		CGALUtils::copyPolyhedron(*poly, alien_poly);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
	}
	else if (auto nef = hybrid.getNefPolyhedron()) {
		assert(nef);
		if (!nef) return nullptr;
		SCOPED_PERFORMANCE_TIMER(
				"createNefPolyhedronFromHybrid(Nef -> Nef through polyhedron: costly)");

		CGALHybridPolyhedron::polyhedron_t poly;
		nef->convert_to_polyhedron(poly);

		CGAL_Polyhedron alien_poly;
		CGALUtils::copyPolyhedron(poly, alien_poly);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
	}
	else {
		assert(!"Invalid hybrid polyhedron state");
		return nullptr;
	}
}
} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE
