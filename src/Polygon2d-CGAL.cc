#include "Polygon2d-CGAL.h"
#include "polyset.h"
#include "printutils.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polygon_2.h>
#include <iostream>

namespace Polygon2DCGAL {

struct FaceInfo
{
	FaceInfo() : nesting_level(42) {}
  int nesting_level;
  bool in_domain() { return nesting_level%2 == 1; }
};


typedef CGAL::Exact_predicates_inexact_constructions_kernel       K;
typedef CGAL::Triangulation_vertex_base_2<K>                      Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo,K>     Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K,Fbb>        Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb>               TDS;
typedef CGAL::Exact_predicates_tag                                Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag>  CDT;
typedef CDT::Point                                                Point;
typedef CGAL::Polygon_2<K>                                        Polygon_2;

void 
mark_domains(CDT &ct, 
             CDT::Face_handle start, 
             int index, 
             std::list<CDT::Edge> &border)
{
  if (start->info().nesting_level != -1) return;

  std::list<CDT::Face_handle> queue;
  queue.push_back(start);

  while (!queue.empty()) {
    auto fh = queue.front();
    queue.pop_front();
    if (fh->info().nesting_level == -1) {
      fh->info().nesting_level = index;
      for (int i = 0; i < 3; i++) {
        CDT::Edge e(fh,i);
        auto n = fh->neighbor(i);
        if (n->info().nesting_level == -1) {
          if (ct.is_constrained(e)) border.push_back(e);
          else queue.push_back(n);
        }
      }
    }
  }
}

// Explore set of facets connected with non constrained edges,
// and attribute to each such set a nesting level.
// We start from facets incident to the infinite vertex, with a nesting
// level of 0. Then we recursively consider the non-explored facets incident 
// to constrained edges bounding the former set and increase the nesting level by 1.
// Facets in the domain are those with an odd nesting level.
void
mark_domains(CDT &cdt)
{
  for(CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it) {
    it->info().nesting_level = -1;
  }

  int index = 0;
  std::list<CDT::Edge> border;
  mark_domains(cdt, cdt.infinite_face(), index++, border);
  while (!border.empty()) {
    CDT::Edge e = border.front();
    border.pop_front();
    CDT::Face_handle n = e.first->neighbor(e.second);
    if (n->info().nesting_level == -1) {
      mark_domains(cdt, n, e.first->info().nesting_level+1, border);
    }
  }
}

}

#define OPENSCAD_CGAL_ERROR_BEGIN \
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION); \
	try {

#define OPENSCAD_CGAL_ERROR_END(errorstr, onerror) \
  } \
	catch (const CGAL::Precondition_exception &e) { \
		PRINTB(errorstr ": %s", e.what()); \
		CGAL::set_error_behaviour(old_behaviour); \
		onerror; \
	} \
	CGAL::set_error_behaviour(old_behaviour);
  

/*!
	Triangulates this polygon2d and returns a 2D PolySet.
*/
PolySet *Polygon2d::tessellate() const
{
	PRINTDB("Polygon2d::tessellate(): %d outlines", this->outlines().size());
	auto polyset = new PolySet(*this);

	Polygon2DCGAL::CDT cdt; // Uses a constrained Delaunay triangulator.
	OPENSCAD_CGAL_ERROR_BEGIN;
	// Adds all vertices, and add all contours as constraints.
	for (const auto &outline : this->outlines()) {
		// Start with last point
		auto prev = cdt.insert({outline.vertices[outline.vertices.size()-1][0], outline.vertices[outline.vertices.size()-1][1]});
		for (const auto &v : outline.vertices) {
			auto curr = cdt.insert({v[0], v[1]});
			if (prev != curr) { // Ignore duplicate vertices
				cdt.insert_constraint(prev, curr);
				prev = curr;
			}
		}
	}
	OPENSCAD_CGAL_ERROR_END("CGAL error in Polygon2d::tesselate()", return nullptr);

	// To extract triangles which is part of our polygon, we need to filter away
	// triangles inside holes.
	mark_domains(cdt);
	for (auto fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) {
		if (fit->info().in_domain()) {
			polyset->append_poly();
			for (int i=0;i<3;i++) {
				polyset->append_vertex(fit->vertex(i)->point()[0],
															 fit->vertex(i)->point()[1],
															 0);
			}
		}
	}
	return polyset;
}
