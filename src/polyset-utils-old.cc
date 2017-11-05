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
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesher_no_edge_refinement_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_criteria_2.h>
#include <CGAL/Mesh_2/Face_badness.h>
#ifdef PREV_NDEBUG
#define NDEBUG PREV_NDEBUG
#endif

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds, CGAL::Exact_predicates_tag> CDT;
//typedef CGAL::Delaunay_mesh_criteria_2<CDT> Criteria;

typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Point CDTPoint;

template <class T> class DummyCriteria
{
public:
	typedef double Quality;
	class Is_bad
	{
public:
		CGAL::Mesh_2::Face_badness operator()(const Quality) const {
			return CGAL::Mesh_2::NOT_BAD;
		}
		CGAL::Mesh_2::Face_badness operator()(const typename T::Face_handle &, Quality &q) const {
			q = 1;
			return CGAL::Mesh_2::NOT_BAD;
		}
	};
	Is_bad is_bad_object() const { return Is_bad(); }
};

namespace PolysetUtils {

// Project all polygons (also back-facing) into a Polygon2d instance.
// It's important to select all faces, since filtering by normal vector here
// will trigger floating point incertainties and cause problems later.
Polygon2d *project(const PolySet &ps) {
	Polygon2d *poly = new Polygon2d;

	for (const auto &p : ps.polygons) {
		Outline2d outline;
		for (const auto &v : p) {
			outline.vertices.push_back(Vector2d(v[0], v[1]));
		}
		poly->addOutline(outline);
	}
	return poly;
}

/* Tessellation of 3d PolySet faces

   This code is for tessellating the faces of a 3d PolySet, assuming that
   the faces are near-planar polygons.

   We do the tessellation by projecting each polygon of the Polyset onto a
   2-d plane, then running a 2d tessellation algorithm on the projected 2d
   polygon. Then we project each of the newly generated 2d 'tiles' (the
   polygons used for tessellation, typically triangles) back up into 3d
   space.

   (in reality as of writing, we dont need to do a back-projection from 2d->3d
   because the algorithm we are using doesn't create any new points, and we can
   just use a 'map' to associate 3d points with 2d points).

   The code assumes the input polygons are simple, non-intersecting, without
   holes, without duplicate input points, and with proper orientation.

   The purpose of this code is originally to fix github issue 349. Our CGAL
   kernel does not accept polygons for Nef_Polyhedron_3 if each of the
   points is not exactly coplanar. "Near-planar" or "Almost planar" polygons
   often occur due to rounding issues on, for example, polyhedron() input.
   By tessellating the 3d polygon into individual smaller tiles that
   are perfectly coplanar (triangles, for example), we can get CGAL to accept
   the polyhedron() input.
 */

typedef enum { XYPLANE, YZPLANE, XZPLANE, NONE } projection_t;

// this is how we make 3d points appear as though they were 2d points to
//the tessellation algorithm.
Vector2d get_projected_point(Vector3d v, projection_t projection) {
	Vector2d v2(0, 0);
	if (projection == XYPLANE) {
		v2.x() = v.x(); v2.y() = v.y();
	}
	else if (projection == XZPLANE) {
		v2.x() = v.x(); v2.y() = v.z();
	}
	else if (projection == YZPLANE) {
		v2.x() = v.y(); v2.y() = v.z();
	}
	return v2;
}

CGAL_Point_3 cgp(Vector3d v) { return CGAL_Point_3(v.x(), v.y(), v.z()); }

/* Find a 'good' 2d projection for a given 3d polygon. the XY, YZ, or XZ
   plane. This is needed because near-planar polygons in 3d can have 'bad'
   projections into 2d. For example if the square 0,0,0 0,1,0 0,1,1 0,0,1
   is projected onto the XY plane you will not get a polygon, you wil get
   a skinny line thing. It's better to project that square onto the yz
   plane.*/
projection_t find_good_projection(PolySet::Polygon pgon) {
	// step 1 - find 3 non-collinear points in the input
	if (pgon.size() < 3) return NONE;
	Vector3d v1, v2, v3;
	v1 = v2 = v3 = pgon[0];
	for (size_t i = 0; i < pgon.size(); i++) {
		if (pgon[i] != v1) {
			v2 = pgon[i]; break;
		}
	}
	if (v1 == v2) return NONE;
	for (size_t i = 0; i < pgon.size(); i++) {
		if (!CGAL::collinear(cgp(v1), cgp(v2), cgp(pgon[i]))) {
			v3 = pgon[i]; break;
		}
	}
	if (CGAL::collinear(cgp(v1), cgp(v2), cgp(v3))) return NONE;
	// step 2 - find which direction is best for projection. planes use
	// the equation ax+by+cz+d = 0. a,b, and c determine the direction the
	// plane is in. we want to find which projection of the 'normal vector'
	// would make the smallest shadow if projected onto the XY, YZ, or XZ
	// plane. 'quadrance' (distance squared) can tell this w/o using sqrt.
	CGAL::Plane_3<CGAL_Kernel3> pl(cgp(v1), cgp(v2), cgp(v3));
	NT3 qxy = pl.a() * pl.a() + pl.b() * pl.b();
	NT3 qyz = pl.b() * pl.b() + pl.c() * pl.c();
	NT3 qxz = pl.c() * pl.c() + pl.a() * pl.a();
	NT3 min = std::min(qxy, std::min(qyz, qxz));
	if (min == qxy) return XYPLANE;
	else if (min == qyz) return YZPLANE;
	return XZPLANE;
}

/* triangulate the given 3d polygon using CGAL's 2d Constrained Delaunay
   algorithm. Project the polygon's points into 2d using the given projection
   before performing the triangulation. This code assumes input polygon is
   simple, no holes, no self-intersections, no duplicate points, and is
   properly oriented. output is a sequence of 3d triangles. */
bool triangulate_polygon(const PolySet::Polygon &pgon, std::vector<PolySet::Polygon> &triangles, projection_t projection)
{
	bool err = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CDT cdt;
		std::vector<Vertex_handle> vhandles;
		std::map<CDTPoint, Vector3d> vertmap;
		CGAL::Orientation original_orientation;
		std::vector<CDTPoint> orienpgon;
		for (size_t i = 0; i < pgon.size(); i++) {
			Vector3d v3 = pgon.at(i);
			Vector2d v2 = get_projected_point(v3, projection);
			CDTPoint cdtpoint = CDTPoint(v2.x(), v2.y());
			vertmap[ cdtpoint ] = v3;
			Vertex_handle vh = cdt.insert(cdtpoint);
			vhandles.push_back(vh);
			orienpgon.push_back(cdtpoint);
		}
		original_orientation = CGAL::orientation_2(orienpgon.begin(), orienpgon.end());
		for (size_t i = 0; i < vhandles.size(); i++) {
			int vindex1 = (i + 0);
			int vindex2 = (i + 1) % vhandles.size();
			cdt.insert_constraint(vhandles[vindex1], vhandles[vindex2]);
		}
		std::list<CDTPoint> list_of_seeds;
		CGAL::refine_Delaunay_mesh_2_without_edge_refinement(cdt,
																												 list_of_seeds.begin(), list_of_seeds.end(), DummyCriteria<CDT>());

		CDT::Finite_faces_iterator fit;
		for (fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); fit++) {
			if (fit->is_in_domain()) {
				CDTPoint p1 = cdt.triangle(fit)[0];
				CDTPoint p2 = cdt.triangle(fit)[1];
				CDTPoint p3 = cdt.triangle(fit)[2];
				Vector3d v1 = vertmap[p1];
				Vector3d v2 = vertmap[p2];
				Vector3d v3 = vertmap[p3];
				PolySet::Polygon pgon;
				if (CGAL::orientation(p1, p2, p3) == original_orientation) {
					pgon.push_back(v1);
					pgon.push_back(v2);
					pgon.push_back(v3);
				}
				else {
					pgon.push_back(v3);
					pgon.push_back(v2);
					pgon.push_back(v1);
				}
				triangles.push_back(pgon);
			}
		}
	}
	catch (const CGAL::Failure_exception &e) {
		// Using failure exception to catch precondition errors for malformed polygons
		// in e.g. CGAL::orientation_2().
		PRINTB("CGAL error in triangulate_polygon(): %s", e.what());
		err = true;
	}
	CGAL::set_error_behaviour(old_behaviour);
	return err;
}

/* Given a 3d PolySet with 'near planar' polygonal faces, Tessellate the
   faces. As of writing, our only tessellation method is Triangulation
   using CGAL's Constrained Delaunay algorithm. This code assumes the input
   polyset has simple polygon faces with no holes, no self intersections, no
   duplicate points, and proper orientation. */
void tessellate_faces(const PolySet &inps, PolySet &outps) {
	int degeneratePolygons = 0;
	for (size_t i = 0; i < inps.polygons.size(); i++) {
		const PolySet::Polygon pgon = inps.polygons[i];
		if (pgon.size() < 3) {
			degeneratePolygons++;
			continue;
		}
		std::vector<PolySet::Polygon> triangles;
		if (pgon.size() == 3) {
			triangles.push_back(pgon);
		}
		else {
			projection_t goodproj = find_good_projection(pgon);
			if (goodproj == NONE) {
				degeneratePolygons++;
				continue;
			}
			bool err = triangulate_polygon(pgon, triangles, goodproj);
			if (err) continue;
		}
		for (size_t j = 0; j < triangles.size(); j++) {
			PolySet::Polygon t = triangles[j];
			outps.append_poly();
			outps.append_vertex(t[0].x(), t[0].y(), t[0].z());
			outps.append_vertex(t[1].x(), t[1].y(), t[1].z());
			outps.append_vertex(t[2].x(), t[2].y(), t[2].z());
		}
	}
	if (degeneratePolygons > 0) PRINT("WARNING: PolySet has degenerate polygons");
}
}
