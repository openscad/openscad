#include "polyset-utils.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "GeometryUtils.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

#include <boost/foreach.hpp>

namespace PolysetUtils {

	// Project all polygons (also back-facing) into a Polygon2d instance.
  // It's important to select all faces, since filtering by normal vector here
	// will trigger floating point incertainties and cause problems later.
	Polygon2d *project(const PolySet &ps) {
		Polygon2d *poly = new Polygon2d;

		BOOST_FOREACH(const Polygon &p, ps.polygons) {
			Outline2d outline;
			BOOST_FOREACH(const Vector3d &v, p) {
				outline.vertices.push_back(Vector2d(v[0], v[1]));
			}
			poly->addOutline(outline);
		}
		return poly;
	}

/* Tessellation of 3d PolySet faces
	 
	 This code is for tessellating the faces of a 3d PolySet, assuming that
	 the faces are near-planar polygons.
	 
	 The purpose of this code is originally to fix github issue 349. Our CGAL
	 kernel does not accept polygons for Nef_Polyhedron_3 if each of the
	 points is not exactly coplanar. "Near-planar" or "Almost planar" polygons
	 often occur due to rounding issues on, for example, polyhedron() input.
	 By tessellating the 3d polygon into individual smaller tiles that
	 are perfectly coplanar (triangles, for example), we can get CGAL to accept
	 the polyhedron() input.
*/
	
/* Given a 3D PolySet with near planar polygonal faces, tessellate the
	 faces. As of writing, our only tessellation method is triangulation
	 using CGAL's Constrained Delaunay algorithm. This code assumes the input
	 polyset has simple polygon faces with no holes.
	 The tessellation will be robust wrt. degenerate and self-intersecting
*/
	void tessellate_faces(const PolySet &inps, PolySet &outps) {
		int degeneratePolygons = 0;
		for (size_t i = 0; i < inps.polygons.size(); i++) {
			const Polygon &pgon = inps.polygons[i];
			if (pgon.size() < 3) {
				degeneratePolygons++;
			}
			else {
				Polygons triangles;
				bool err = GeometryUtils::tessellatePolygon(pgon, triangles);
				// Empty triangles tend to happen quite often,
				// probably due to previous floating point conversion, so
				// we don't issue any warnings.
				if (!triangles.empty()) {
					// ..and pass to the output polyhedron
					BOOST_FOREACH(const Polygon &t, triangles) {
						outps.append_poly();
						outps.append_vertex(t[0]);
						outps.append_vertex(t[1]);
						outps.append_vertex(t[2]);
					}
				}
			}
		}
		if (degeneratePolygons > 0) PRINT("WARNING: PolySet has degenerate polygons");
	}

	bool is_approximately_convex(const PolySet &ps) {
#ifdef ENABLE_CGAL
		return CGALUtils::is_approximately_convex(ps);
#else
		return false;
#endif
	}

}
