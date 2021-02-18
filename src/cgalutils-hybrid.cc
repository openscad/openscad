// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

#include "CGAL_Nef_polyhedron.h"
#include "polyset-utils.h"

namespace CGALUtils {

std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromGeometry(const Geometry &geom)
{
	if (auto poly = dynamic_cast<const CGALHybridPolyhedron *>(&geom)) {
		return make_shared<CGALHybridPolyhedron>(*poly);
	}
	else if (auto ps = dynamic_cast<const PolySet *>(&geom)) {
		auto polyhedron = make_shared<CGALHybridPolyhedron::polyhedron_t>();

#ifdef FAST_CSG_LEGACY_TESSELATION
		PolySet ps_tri(3, ps->convexValue());
		ps_tri.setConvexity(ps->getConvexity());
		PolysetUtils::tessellate_faces(*ps, ps_tri);

		auto err = createPolyhedronFromPolySet(ps_tri, *polyhedron, /* invert_orientation */ false);
		assert(!err);
#else
		auto err = createPolyhedronFromPolySet(*ps, *polyhedron, /* invert_orientation */ false);
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
	else if (auto nef = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
		assert(nef->p3);

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
		LOG(message_group::Warning, Location::NONE, "", "Unsupported geometry format.");
		return nullptr;
	}
}

shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron &hybrid)
{
	if (auto poly = hybrid.getPolyhedron()) {
		CGAL_Polyhedron alien_poly;
		copyPolyhedron(*poly, alien_poly);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
	}
	else if (auto nef = hybrid.getNefPolyhedron()) {
		assert(nef);
		if (!nef) return nullptr;

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
