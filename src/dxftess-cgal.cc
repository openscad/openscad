#include "openscad.h"
#include "printutils.h"

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

void dxf_tesselate(PolySet *ps, DxfData *dxf, double rot, bool up, bool do_triangle_splitting, double h)
{
	CDT cdt;

  //       <pathidx,pointidx>
	Grid3d< QPair<int,int> > point_to_path(GRID_FINE);

	for (int i = 0; i < dxf->paths.count(); i++) {
		if (!dxf->paths[i].is_closed)
			continue;
		Vertex_handle first, prev;
		for (int j = 1; j < dxf->paths[i].points.count(); j++) {
			double x = dxf->paths[i].points[j]->x;
			double y = dxf->paths[i].points[j]->y;
			point_to_path.data(x, y, h) = QPair<int,int>(i, j);
			Vertex_handle vh = cdt.insert(CDTPoint(x, y));
			if (j == 1) {
				first = vh;
			}
			else {
				cdt.insert_constraint(prev, vh);
			}
			prev = vh;
		}
 		cdt.insert_constraint(prev, first);
	}

	std::list<CDTPoint> list_of_seeds;
// FIXME: Give hints about holes here
// 	list_of_seeds.push_back(CDTPoint(-1, -1));
// 	list_of_seeds.push_back(CDTPoint(20, 50));
	CGAL::refine_Delaunay_mesh_2_without_edge_refinement(cdt, list_of_seeds.begin(), list_of_seeds.end(), 
															 DummyCriteria<CDT>());
	CDT::Finite_faces_iterator iter = cdt.finite_faces_begin();
  for( ; iter != cdt.finite_faces_end(); ++iter) {
		if (!iter->is_in_domain()) continue;
		ps->append_poly();

		int path[3], point[3];
		for (int i=0;i<3;i++) {
			int idx = up ? i : (2-i);
			double px = iter->vertex(idx)->point()[0];
			double py = iter->vertex(idx)->point()[1];
			ps->append_vertex(px *  cos(rot*M_PI/180) + py * sin(rot*M_PI/180),
												px * -sin(rot*M_PI/180) + py * cos(rot*M_PI/180),
												h);
			path[i] = point_to_path.data(px, py, h).first;
			point[i] = point_to_path.data(px, py, h).second;
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
