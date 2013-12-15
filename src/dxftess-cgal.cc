#include "printutils.h"
#include "dxftess.h"
#include "dxfdata.h"
#include "polyset.h"
#include "grid.h"
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
typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds, CGAL::Exact_predicates_tag > CDT;
//typedef CGAL::Delaunay_mesh_criteria_2<CDT> Criteria;

typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Point CDTPoint;

#include <boost/unordered_map.hpp>

template <class T> class DummyCriteria {
public:
	typedef double Quality;
	class Is_bad {
	public:
		CGAL::Mesh_2::Face_badness operator()(const Quality) const {
			return CGAL::Mesh_2::NOT_BAD;
		}
		CGAL::Mesh_2::Face_badness operator()(const typename T::Face_handle&, Quality&q) const {
			q = 1;
			return CGAL::Mesh_2::NOT_BAD;
		}
	};
	Is_bad is_bad_object() const { return Is_bad(); }
};

struct triangle {
	struct { double x, y; } p[3];
	bool is_inner, is_marked;
};

struct point_info_t
{
	double x, y;
	int pathidx, pointidx;
	int max_pointidx_in_path;
	std::vector<int> triangles;

	struct point_info_t *neigh_next;
	struct point_info_t *neigh_prev;

	point_info_t(double x, double y, int a, int b, int c) :
			x(x), y(y), pathidx(a), pointidx(b), max_pointidx_in_path(c) { }
	point_info_t() : x(0), y(0), pathidx(-1), pointidx(-1), max_pointidx_in_path(-1) { }
};

typedef std::pair<point_info_t*,point_info_t*> edge_t;

void mark_inner_outer(std::vector<struct triangle> &tri, Grid2d<point_info_t> &point_info,
											boost::unordered_map<edge_t,int> &edge_to_triangle,
											boost::unordered_map<edge_t,int> &edge_to_path, int idx, bool inner)
{
	if (tri[idx].is_marked)
		return;

	tri[idx].is_inner = inner;
	tri[idx].is_marked = true;

	point_info_t *p[3] = {
		&point_info.data(tri[idx].p[0].x, tri[idx].p[0].y),
		&point_info.data(tri[idx].p[1].x, tri[idx].p[1].y),
		&point_info.data(tri[idx].p[2].x, tri[idx].p[2].y)
	};

	edge_t edges[3] = {
		edge_t(p[1], p[0]),
		edge_t(p[2], p[1]),
		edge_t(p[0], p[2])
	};

	for (int i = 0; i < 3; i++) {
		if (edge_to_triangle.find(edges[i]) != edge_to_triangle.end()) {
			bool next_inner = (edge_to_path.find(edges[i]) != edge_to_path.end()) ? !inner : inner;
			mark_inner_outer(tri, point_info, edge_to_triangle, edge_to_path,
					edge_to_triangle[edges[i]], next_inner);
		}
	}
}

void dxf_tesselate(PolySet *ps, DxfData &dxf, double rot, Vector2d scale, bool up, bool /* do_triangle_splitting */, double h)
{
	CDT cdt;

	std::vector<struct triangle> tri;
	Grid2d<point_info_t> point_info(GRID_FINE);
	boost::unordered_map<edge_t,int> edge_to_triangle;
	boost::unordered_map<edge_t,int> edge_to_path;
	int duplicate_vertices = 0;

	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {

	// read path data and copy all relevant infos
	for (size_t i = 0; i < dxf.paths.size(); i++)
	{
		if (!dxf.paths[i].is_closed)
			continue;

		Vertex_handle first, prev;
		struct point_info_t *first_pi = NULL, *prev_pi = NULL;
		for (size_t j = 1; j < dxf.paths[i].indices.size(); j++)
		{
			double x = dxf.points[dxf.paths[i].indices[j]][0];
			double y = dxf.points[dxf.paths[i].indices[j]][1];

			if (point_info.has(x, y)) {
				// FIXME: How can the same path set contain the same point twice?
				// ..maybe it would be better to assert here. But this would
				// break compatibility with the glu tesselator that handled such
				// cases just fine.
				duplicate_vertices++;
				continue;
			}

			struct point_info_t *pi = &point_info.align(x, y);
			*pi = point_info_t(x, y, i, j, dxf.paths[i].indices.size()-1);

			Vertex_handle vh = cdt.insert(CDTPoint(x, y));
			if (first_pi == NULL) {
				first_pi = pi;
				first = vh;
			} else {
				prev_pi->neigh_next = pi;
				pi->neigh_prev = prev_pi;
				edge_to_path[edge_t(prev_pi, pi)] = 1; 
				edge_to_path[edge_t(pi, prev_pi)] = 1; 
				cdt.insert_constraint(prev, vh);
			}
			prev_pi = pi;
			prev = vh;
		}

		if (first_pi != NULL && first_pi != prev_pi)
		{
			prev_pi->neigh_next = first_pi;
			first_pi->neigh_prev = prev_pi;

			edge_to_path[edge_t(first_pi, prev_pi)] = 1; 
			edge_to_path[edge_t(prev_pi, first_pi)] = 1; 

			cdt.insert_constraint(prev, first);
		}
	}

	if ( duplicate_vertices > 0 ) {
		PRINT( "WARNING: Duplicate vertices and/or intersecting lines found during DXF Tessellation." );
		PRINT( "WARNING: Modify the polygon to be a Simple Polygon. Render is incomplete." );
	}

	}
	catch (const CGAL::Assertion_exception &e) {
		PRINTB("CGAL error in dxf_tesselate(): %s", e.what());
		CGAL::set_error_behaviour(old_behaviour);
		return;
	}
	CGAL::set_error_behaviour(old_behaviour);

	// run delaunay triangulation
	std::list<CDTPoint> list_of_seeds;
	CGAL::refine_Delaunay_mesh_2_without_edge_refinement(cdt,
			list_of_seeds.begin(), list_of_seeds.end(), DummyCriteria<CDT>());

	// copy triangulation results
	CDT::Finite_faces_iterator iter = cdt.finite_faces_begin();
	for(; iter != cdt.finite_faces_end(); ++iter)
	{
		if (!iter->is_in_domain())
			continue;

		int idx = tri.size();
		tri.push_back(triangle());

		point_info_t *pi[3];
		for (int i=0; i<3; i++) {
			double px = iter->vertex(i)->point()[0];
			double py = iter->vertex(i)->point()[1];
			pi[i] = &point_info.align(px, py);
			pi[i]->triangles.push_back(idx);
			tri[idx].p[i].x = px;
			tri[idx].p[i].y = py;
		}

		edge_to_triangle[edge_t(pi[0], pi[1])] = idx;
		edge_to_triangle[edge_t(pi[1], pi[2])] = idx;
		edge_to_triangle[edge_t(pi[2], pi[0])] = idx;
	}

	// mark trianlges as inner/outer
	while (1)
	{
		double far_left_x = 0;
		struct point_info_t *far_left_p = NULL;

		for (size_t i = 0; i < tri.size(); i++)
		{
			if (tri[i].is_marked)
				continue;

			for (int j = 0; j < 3; j++) {
				double x = tri[i].p[j].x;
				double y = tri[i].p[j].y;
				if (far_left_p == NULL || x < far_left_x) {
					far_left_x = x;
					far_left_p = &point_info.data(x, y);
				}
			}
		}

		if (far_left_p == NULL)
			break;

		// find one inner triangle and run recursive marking
		for (size_t i = 0; i < far_left_p->triangles.size(); i++)
		{
			int idx = far_left_p->triangles[i];

			if (tri[idx].is_marked)
				continue;

			point_info_t *p0 = &point_info.data(tri[idx].p[0].x, tri[idx].p[0].y);
			point_info_t *p1 = &point_info.data(tri[idx].p[1].x, tri[idx].p[1].y);
			point_info_t *p2 = &point_info.data(tri[idx].p[2].x, tri[idx].p[2].y);
			point_info_t *mp = NULL, *np1 = NULL, *np2 = NULL, *tp = NULL;

			if (p0 == far_left_p)
				mp = p0, np1 = p1, np2 = p2;
			else if (p1 == far_left_p)
				mp = p1, np1 = p0, np2 = p2;
			else if (p2 == far_left_p)
				mp = p2, np1 = p0, np2 = p1;
			else
				continue;

			if (mp->neigh_next == np2 || mp->neigh_prev == np1) {
				point_info_t *t = np1;
				np1 = np2;
				np2 = t;
			}

			if (mp->neigh_next == np1 && mp->neigh_prev == np2) {
				mark_inner_outer(tri, point_info, edge_to_triangle, edge_to_path, idx, true);
				goto found_and_marked_inner;
			}

			if (mp->neigh_next == np1)
				tp = np2;

			if (mp->neigh_prev == np2)
				tp = np1;

			if (tp != NULL) {
				double z0 = (mp->neigh_next->x - mp->x) * (mp->neigh_prev->y - mp->y) -
						(mp->neigh_prev->x - mp->x) * (mp->neigh_next->y - mp->y);
				double z1 = (mp->neigh_next->x - mp->x) * (tp->y - mp->y) -
						(tp->x - mp->x) * (mp->neigh_next->y - mp->y);
				double z2 = (tp->x - mp->x) * (mp->neigh_prev->y - mp->y) -
						(mp->neigh_prev->x - mp->x) * (tp->y - mp->y);
				if ((z0 < 0 && z1 < 0 && z2 < 0) || (z0 > 0 && z1 > 0 && z2 > 0)) {
					mark_inner_outer(tri, point_info, edge_to_triangle, edge_to_path, idx, true);
					goto found_and_marked_inner;
				}
			}
		}

		// far left point is in the middle of a vertical segment
		// -> it is ok to use any unmarked triangle connected to this point
		for (size_t i = 0; i < far_left_p->triangles.size(); i++)
		{
			int idx = far_left_p->triangles[i];

			if (tri[idx].is_marked)
				continue;

			mark_inner_outer(tri, point_info, edge_to_triangle, edge_to_path, idx, true);
			break;
		}

	found_and_marked_inner:;
	}

	// add all inner triangles to target polyset
	for(size_t i = 0; i < tri.size(); i++)
	{
		if (!tri[i].is_inner)
			continue;

		ps->append_poly();
		int path[3], point[3];
		for (int j=0;j<3;j++) {
			int idx = up ? j : (2-j);
			double px = tri[i].p[idx].x;
			double py = tri[i].p[idx].y;
			ps->append_vertex(scale[0] * (px * cos(rot*M_PI/180) + py * sin(rot*M_PI/180)),
												scale[1] * (px * -sin(rot*M_PI/180) + py * cos(rot*M_PI/180)), h);
			path[j] = point_info.data(px, py).pathidx;
			point[j] = point_info.data(px, py).pointidx;
		}

		if (path[0] == path[1] && point[0] == 1 && point[1] == 2)
			dxf.paths[path[0]].is_inner = up;
		if (path[0] == path[1] && point[0] == 2 && point[1] == 1)
			dxf.paths[path[0]].is_inner = !up;
		if (path[1] == path[2] && point[1] == 1 && point[2] == 2)
			dxf.paths[path[1]].is_inner = up;
		if (path[1] == path[2] && point[1] == 2 && point[2] == 1)
			dxf.paths[path[1]].is_inner = !up;

		if (path[2] == path[0] && point[2] == 1 && point[0] == 2)
			dxf.paths[path[2]].is_inner = up;
		if (path[2] == path[0] && point[2] == 2 && point[0] == 1)
			dxf.paths[path[2]].is_inner = !up;
	}
}


/* Tessellation of 3d PolySet with near-planar polygons.

We do this by projecting each polygon of the Polyset onto a 2-d plane,
then running a tessellation algorithm on the projected polygon. Then we
project the generated 2d triangles back up into 3d space.

(in reality as of writing, we dont need to do a back-projection from 2d->3d
because the algorithm we are using doesn't create any new points, and we can
just use a 'map' to associate 3d points to 2d points).

The code assumes the input polygons are simple, non-intersecting, without
holes, and without duplicate input points.

For more info, please see github #issue349. This code enables
polyhedron() to use near-planar polygons rather than perfectly planar
polygons. */

typedef enum { XYPLANE, YZPLANE, XZPLANE, NONE } projection_t;

// this is how we make 3d points appear as though they were 2d points to
//the tessellation algorithm.
Vector2d get_projected_point( Vector3d v, projection_t projection ) {
	Vector2d v2(0,0);
	if (projection==XYPLANE) { v2.x() = v.x(); v2.y() = v.y(); }
	else if (projection==XZPLANE) { v2.x() = v.x(); v2.y() = v.z(); }
	else if (projection==YZPLANE) { v2.x() = v.y(); v2.y() = v.z(); }
	return v2;
}

CGAL_Point_3 cgp( Vector3d v ) { return CGAL_Point_3( v.x(), v.y(), v.z() ); }

/* Find a 'good' 2d projection for a given 3d polygon. the XY, YZ, or XZ 
plane. This is needed because near-planar polygons in 3d can have 'bad' 
projections into 2d. For example if the square 0,0,0 0,1,0 0,1,1 0,0,1 
is projected onto the XY plane you will not get a polygon, you wil get 
a skinny line thing. It's better to project that square onto the yz 
plane.*/
projection_t find_good_projection( PolySet::Polygon pgon ) {
	// step 1 - find 3 non-collinear points in the input
	if (pgon.size()<3) return NONE;
	Vector3d v1,v2,v3;
	v1 = v2 = v3 = pgon[0];
	for (size_t i=0;i<pgon.size();i++) {
		if (pgon[i]!=v1) { v2=pgon[i]; break; }
	}
	if (v1==v2) return NONE;
	for (size_t i=0;i<pgon.size();i++) {
		if (!CGAL::collinear( cgp(v1), cgp(v2), cgp(pgon[i]) )) {
			v3=pgon[i]; break;
		}
	}
	if (CGAL::collinear( cgp(v1), cgp(v2), cgp(v3) ) ) return NONE;
	// step 2 - find which direction is best for projection. planes use
	// the equation ax+by+cz+d = 0. a,b, and c determine the direction the
	// plane is in. we want to find which projection of the 'normal vector'
	// would make the smallest shadow if projected onto the XY, YZ, or XZ
	// plane. 'quadrance' (distance squared) can tell this w/o using sqrt.
	CGAL::Plane_3<CGAL_Kernel3> pl( cgp(v1), cgp(v2), cgp(v3) );
	NT3 qxy = pl.a()*pl.a()+pl.b()*pl.b();
        NT3 qyz = pl.b()*pl.b()+pl.c()*pl.c();
        NT3 qxz = pl.c()*pl.c()+pl.a()*pl.a();
	NT3 min = std::min(qxy,std::min(qyz,qxz));
	if (min==qxy) return XYPLANE;
        else if (min==qyz) return YZPLANE;
        return XZPLANE;
}

/* triangulate the given polygon using CGAL's Constrained Delaunay 
algorithm. project the polygon's points using the given projection 
before performing the triangulation. this code assumes input polygon is 
simple, no holes, no self-intersections, no duplicate points, and 
properly oriented. */
void triangulate_polygon( const PolySet::Polygon &pgon, std::vector<PolySet::Polygon> &triangles, projection_t projection )
{
	CDT cdt;
	std::vector<Vertex_handle> vhandles;
	std::map<CDTPoint,Vector3d> vertmap;
	CGAL::Orientation original_orientation;
	std::vector<CDTPoint> orpgon;
	for (size_t i = 0; i < pgon.size(); i++) {
		Vector3d v3 = pgon.at(i);
		Vector2d v2 = get_projected_point( v3, projection );
		CDTPoint cdtpoint = CDTPoint(v2.x(),v2.y());
		vertmap[ cdtpoint ] = v3;
		Vertex_handle vh = cdt.insert( cdtpoint );
		vhandles.push_back(vh);
		orpgon.push_back( cdtpoint );
	}
	original_orientation = CGAL::orientation_2( orpgon.begin(),orpgon.end() );
	for (size_t i = 0; i < vhandles.size(); i++ ) {
		int vindex1 = (i+0);
		int vindex2 = (i+1)%vhandles.size();
		cdt.insert_constraint( vhandles[vindex1], vhandles[vindex2] );
	}
	std::list<CDTPoint> list_of_seeds;
	CGAL::refine_Delaunay_mesh_2_without_edge_refinement(cdt,
	  list_of_seeds.begin(), list_of_seeds.end(), DummyCriteria<CDT>());

	CDT::Finite_faces_iterator fit;
        for( fit=cdt.finite_faces_begin(); fit!=cdt.finite_faces_end(); fit++ )
        {
                if(fit->is_in_domain()) {
			CDTPoint p1 = cdt.triangle( fit )[0];
			CDTPoint p2 = cdt.triangle( fit )[1];
			CDTPoint p3 = cdt.triangle( fit )[2];
			Vector3d v1 = vertmap[p1];
			Vector3d v2 = vertmap[p2];
			Vector3d v3 = vertmap[p3];
			PolySet::Polygon pgon;
			if (CGAL::orientation(p1,p2,p3)==original_orientation) {
				pgon.push_back(v1);
				pgon.push_back(v2);
				pgon.push_back(v3);
			} else {
				pgon.push_back(v3);
				pgon.push_back(v2);
				pgon.push_back(v1);
			}
			triangles.push_back( pgon );
                }
        }
}

/* Given a 3d PolySet with 'near planar' polygonal faces, Tessellate the
faces. As of writing, our only tessellation method is Triangulation
using CGAL's Constrained Delaunay algorithm. This code assumes the input
polyset has simple polygon faces with no holes, no self intersections, and no
duplicate points. */
void tessellate_3d_faces( const PolySet &inps, PolySet &outps ) {
	PRINTB("tess 3d %i",inps.polygons.size());
	PRINTB("%s < input ps",inps.dump());
        for (size_t i = 0; i < inps.polygons.size(); i++) {
                const PolySet::Polygon pgon = inps.polygons[i];
		if (pgon.size()<3) continue;
		std::vector<PolySet::Polygon> triangles;
		projection_t projection = find_good_projection( pgon );
		triangulate_polygon( pgon, triangles, projection );
		for (size_t j=0;j<triangles.size();j++) {
			PolySet::Polygon t = triangles[j];
			outps.append_poly();
			outps.append_vertex(t[0].x(),t[0].y(),t[0].z());
			outps.append_vertex(t[1].x(),t[1].y(),t[1].z());
			outps.append_vertex(t[2].x(),t[2].y(),t[2].z());
		}
        }
	PRINTB("tess 3d done %i",outps.polygons.size());
	PRINTB("%s < output ps",outps.dump());
}

// End of the Tessellation of PolySet polygons

