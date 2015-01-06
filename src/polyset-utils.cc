#include "polyset-utils.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "printutils.h"
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
#ifdef ENABLE_CGAL
		int degeneratePolygons = 0;
		for (size_t i = 0; i < inps.polygons.size(); i++) {
			const Polygon pgon = inps.polygons[i];
			if (pgon.size() < 3) {
				degeneratePolygons++;
				continue;
			}
			std::vector<Polygon> triangles;
			if (pgon.size() == 3) {
				triangles.push_back(pgon);
			}
			else {
				// Build a data structure that CGAL accepts
				PolygonK cgalpoints;
				BOOST_FOREACH(const Vector3d &v, pgon) {
					cgalpoints.push_back(Vertex3K(v[0], v[1], v[2]));
				}

				bool err = CGALUtils::tessellatePolygon(cgalpoints, triangles);
			}

			// ..and pass to the output polyhedron
			for (size_t j=0;j<triangles.size();j++) {
				Polygon t = triangles[j];
				outps.append_poly();
				outps.append_vertex(t[0].x(),t[0].y(),t[0].z());
				outps.append_vertex(t[1].x(),t[1].y(),t[1].z());
				outps.append_vertex(t[2].x(),t[2].y(),t[2].z());
			}
		}
		if (degeneratePolygons > 0) PRINT("WARNING: PolySet has degenerate polygons");
#else
		assert(false);
#endif
	}

	bool is_approximately_convex(const PolySet &ps) {
#ifdef ENABLE_CGAL
		return CGALUtils::is_approximately_convex(ps);
#else
		return false;
#endif
	}

}
