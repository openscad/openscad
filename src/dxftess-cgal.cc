#include "printutils.h"
#include "dxftess.h"
#include "dxfdata.h"
#include "polyset.h"
#include "grid.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesher_no_edge_refinement_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_criteria_2.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
//typedef CGAL::Delaunay_mesh_criteria_2<CDT> Criteria;

typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Point CDTPoint;

#include <CGAL/Mesh_2/Face_badness.h>

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
	int pathidx, pointidx;
	int max_pointidx_in_path;
	QList<int> triangles;

	struct point_info_t *neigh_up;
	struct point_info_t *neigh_down;

	point_info_t() : pathidx(-1), pointidx(-1), max_pointidx_in_path(-1) { }
	point_info_t(int a, int b, int c) : pathidx(a), pointidx(b), max_pointidx_in_path(c) { }
};

#if 0
void mark_inner_outer(QList<struct triangle> &t, Grid2d<point_info_t> &p, int idx, bool inner)
{
	if (t[idx].is_marked)
		return;

	if (inner)
		t[idx].is_inner = true;

	FIXME
}
#endif

void dxf_tesselate(PolySet *ps, DxfData *dxf, double rot, bool up, bool /* do_triangle_splitting */, double h)
{
	CDT cdt;

	QList<struct triangle> tri;
	Grid2d<point_info_t> point_info(GRID_FINE);

	double far_left_x = 0;
	struct point_info_t *far_left_p = NULL;

	for (int i = 0; i < dxf->paths.count(); i++)
	{
		if (!dxf->paths[i].is_closed)
			continue;

		Vertex_handle first, prev;
		struct point_info_t *first_pi = NULL, *prev_pi = NULL;
		for (int j = 1; j < dxf->paths[i].points.count(); j++)
		{
			double x = dxf->paths[i].points[j]->x;
			double y = dxf->paths[i].points[j]->y;

			struct point_info_t *pi = &point_info.align(x, y);
			*pi = point_info_t(i, j, dxf->paths[i].points.count()-1);

			if (j == 1) {
				first_pi = pi;
			} else {
				prev_pi->neigh_up = pi;
				pi->neigh_down = prev_pi;
			}
			prev_pi = pi;

			if (far_left_p == NULL || x < far_left_x) {
				far_left_x = x;
				far_left_p = &point_info.data(x, y);
			}

			Vertex_handle vh = cdt.insert(CDTPoint(x, y));
			if (j == 1) {
				first = vh;
			} else {
				cdt.insert_constraint(prev, vh);
			}
			prev = vh;
		}

		prev_pi->neigh_up = first_pi;
		first_pi->neigh_down = prev_pi;

 		cdt.insert_constraint(prev, first);
	}

	std::list<CDTPoint> list_of_seeds;
	CGAL::refine_Delaunay_mesh_2_without_edge_refinement(cdt,
			list_of_seeds.begin(), list_of_seeds.end(), DummyCriteria<CDT>());

	CDT::Finite_faces_iterator iter = cdt.finite_faces_begin();
	for(; iter != cdt.finite_faces_end(); ++iter)
	{
		if (!iter->is_in_domain())
			continue;

		int idx = tri.size();
		tri.append(triangle());

		for (int i=0; i<3; i++) {
			double px = iter->vertex(i)->point()[0];
			double py = iter->vertex(i)->point()[1];
			point_info.align(px, py).triangles.append(idx);
			tri[idx].p[i].x = px;
			tri[idx].p[i].y = py;
		}
	}

#if 0
	for (int i = 0; i < far_left_p->triangles.size(); i++)
	{
		int idx = far_left_p->triangles[i];
		point_info_t *p0 = &point_info.data(tri[idx].p[0].x, tri[idx].p[0].y);
		point_info_t *p1 = &point_info.data(tri[idx].p[1].x, tri[idx].p[1].y);
		point_info_t *p2 = &point_info.data(tri[idx].p[2].x, tri[idx].p[2].y);
		point_info_t *mp = NULL, *np1 = NULL, *np2 = NULL;

		if (p0 == far_left_p)
			mp = p0, np1 = p1, np2 = p2;
		else if (p1 == far_left_p)
			mp = p1, np1 = p0, np2 = p2;
		else if (p2 == far_left_p)
			mp = p2, np1 = p0, np2 = p1;
		else
			continue;

		if (mp->neigh_up == np2 || mp->neigh_down == np1) {
			point_info_t *t = np1;
			np1 = np2;
			np2 = t;
		}

		if (mp->neigh_up == np1 && mp->neigh_down == np2) {
			mark_inner_outer(tri, point_info, idx, true);
			break;
		}

		if (mp->neigh_up == np1) {
			FIXME
		}

		if (mp->neigh_up == np1) {
			FIXME
		}
	}
#endif

	for(int i = 0; i < tri.size(); i++)
	{
		// if (!tri[i].is_inner)
		//	continue;

		ps->append_poly();
		int path[3], point[3];
		for (int j=0;j<3;j++) {
			int idx = up ? j : (2-j);
			double px = tri[i].p[idx].x;
			double py = tri[i].p[idx].y;
			ps->append_vertex(px * cos(rot*M_PI/180) + py * sin(rot*M_PI/180),
					px * -sin(rot*M_PI/180) + py * cos(rot*M_PI/180), h);
			path[j] = point_info.data(px, py).pathidx;
			point[j] = point_info.data(px, py).pointidx;
		}

		if (path[0] == path[1] && point[0] == 1 && point[1] == 2)
			dxf->paths[path[0]].is_inner = up;
		if (path[0] == path[1] && point[0] == 2 && point[1] == 1)
			dxf->paths[path[0]].is_inner = !up;
		if (path[1] == path[2] && point[1] == 1 && point[2] == 2)
			dxf->paths[path[1]].is_inner = up;
		if (path[1] == path[2] && point[1] == 2 && point[2] == 1)
			dxf->paths[path[1]].is_inner = !up;

		if (path[2] == path[0] && point[2] == 1 && point[0] == 2)
			dxf->paths[path[2]].is_inner = up;
		if (path[2] == path[0] && point[2] == 2 && point[0] == 1)
			dxf->paths[path[2]].is_inner = !up;
	}
}
