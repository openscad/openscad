#include "polyset-utils.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "printutils.h"
#include "cgal.h"

#ifdef NDEBUG
#define PREV_NDEBUG NDEBUG
#undef NDEBUG
#endif
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_2_filtered_projection_traits_3.h>
#ifdef PREV_NDEBUG
#define NDEBUG PREV_NDEBUG
#endif

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_2_filtered_projection_traits_3<K> Projection;
typedef CGAL::Triangulation_data_structure_2 <
	CGAL::Triangulation_vertex_base_2<Projection>,
	CGAL::Constrained_triangulation_face_base_2<Projection> > Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<Projection, Tds, CGAL::Exact_predicates_tag> CDT;

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
	 The tessellation will  be robust wrt. degenerate and self-intersecting
*/
	void tessellate_faces(const PolySet &inps, PolySet &outps) {
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
				std::vector<K::Point_3> cgalpoints;
				BOOST_FOREACH(const Vector3d &v, pgon) {
					cgalpoints.push_back(K::Point_3(v[0], v[1], v[2]));
				}
				// Calculate best guess at face normal using Newell's method
				K::Vector_3 normal;
				CGAL::normal_vector_newell_3(cgalpoints.begin(), cgalpoints.end(), normal);

				// Pass the normal vector to the (undocumented)
				// CGAL::Triangulation_2_filtered_projection_traits_3. This
				// trait deals with projection from 3D to 2D using the normal
				// vector as a hint, and allows for near-planar polygons to be passed to
				// the Constrained Delaunay Triangulator.
				Projection actualProjection(normal);
				CDT cdt(actualProjection);
				for (size_t i=0;i<cgalpoints.size(); i++) {
					cdt.insert_constraint(cgalpoints[i], cgalpoints[(i+1)%cgalpoints.size()]);
				}
				
				// Iterate over the resulting faces
				CDT::Finite_faces_iterator fit;
				for (fit=cdt.finite_faces_begin(); fit!=cdt.finite_faces_end(); fit++) {
					Polygon pgon;
					for (int i=0;i<3;i++) {
						K::Point_3 v = cdt.triangle(fit)[i];
						pgon.push_back(Vector3d(v.x(), v.y(), v.z()));
					}
					triangles.push_back(pgon);
				}
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
	}
}
