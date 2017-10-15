#include "cgalutils.h"
//#include "cgal.h"
//#include "tess.h"

#ifdef NDEBUG
#define PREV_NDEBUG NDEBUG
#undef NDEBUG
#endif
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#if CGAL_VERSION_NR >= CGAL_VERSION_NUMBER(4,11,0)
  #include <CGAL/Triangulation_2_projection_traits_3.h>
#else
  #include <CGAL/Triangulation_2_filtered_projection_traits_3.h>
#endif
#include <CGAL/Triangulation_face_base_with_info_2.h>
#ifdef PREV_NDEBUG
#define NDEBUG PREV_NDEBUG
#endif

struct FaceInfo {
  int nesting_level;
  bool in_domain() { return nesting_level%2 == 1; }
};

typedef CGAL::Triangulation_2_filtered_projection_traits_3<K> Projection;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo, K> Fbb;
typedef CGAL::Triangulation_data_structure_2<
	CGAL::Triangulation_vertex_base_2<Projection>,
	CGAL::Constrained_triangulation_face_base_2<Projection, Fbb>> Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<
	Projection, Tds, CGAL::Exact_predicates_tag> CDT;


static void  mark_domains(CDT &ct, 
													CDT::Face_handle start, 
													int index, 
													std::list<CDT::Edge>& border)
{
  if (start->info().nesting_level != -1) return;
  std::list<CDT::Face_handle> queue;
  queue.push_back(start);
  while (!queue.empty()) {
    CDT::Face_handle fh = queue.front();
    queue.pop_front();
    if (fh->info().nesting_level == -1) {
      fh->info().nesting_level = index;
      for (int i = 0; i < 3; i++) {
        CDT::Edge e(fh,i);
        CDT::Face_handle n = fh->neighbor(i);
        if (n->info().nesting_level == -1) {
          if (ct.is_constrained(e)) border.push_back(e);
          else queue.push_back(n);
        }
      }
    }
  }
}


//explore set of facets connected with non constrained edges,
//and attribute to each such set a nesting level.
//We start from facets incident to the infinite vertex, with a nesting
//level of 0. Then we recursively consider the non-explored facets incident 
//to constrained edges bounding the former set and increase the nesting level by 1.
//Facets in the domain are those with an odd nesting level.
static void mark_domains(CDT& cdt)
{
  for(CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it) {
    it->info().nesting_level = -1;
  }
  std::list<CDT::Edge> border;
  mark_domains(cdt, cdt.infinite_face(), 0, border);
  while (!border.empty()) {
    CDT::Edge e = border.front();
    border.pop_front();
    CDT::Face_handle n = e.first->neighbor(e.second);
    if (n->info().nesting_level == -1) {
      mark_domains(cdt, n, e.first->info().nesting_level+1, border);
    }
  }
}

namespace CGALUtils {
	/*!
		polygons define a polygon, potentially with holes. Each contour
		should be added as a separate PolygonK instance.
		The tessellator will handle almost planar polygons.
		
		If the normal is given, we will assume this as the normal vector of the polygon.
		Otherwise, we will try to calculate it using Newell's method.

		The resulting triangles is added to the given triangles vector.
	*/
	bool tessellatePolygonWithHoles(const PolyholeK &polygons,
																	Polygons &triangles,
																	const K::Vector_3 *normal)
	{
		// No polygon. FIXME: Will this ever happen or can we assert here?
		if (polygons.empty()) return false;

		// No hole
		if (polygons.size() == 1) return tessellatePolygon(polygons.front(), triangles, normal);
		
		K::Vector_3 normalvec;
		if (normal) {
			normalvec = *normal;
		}
		else {
			// Calculate best guess at face normal using Newell's method
			CGAL::normal_vector_newell_3(polygons.front().begin(), polygons.front().end(), normalvec);
		}
		double sqrl = normalvec.squared_length();
		if (sqrl > 0.0) normalvec = normalvec / sqrt(sqrl);

		// Pass the normal vector to the (undocumented)
		// CGAL::Triangulation_2_filtered_projection_traits_3. This
		// trait deals with projection from 3D to 2D using the normal
		// vector as a hint, and allows for near-planar polygons to be passed to
		// the Constrained Delaunay Triangulator.
		Projection actualProjection(normalvec);
		CDT cdt(actualProjection);
		for(const auto &poly : polygons) {
			for (size_t i=0;i<poly.size(); i++) {
				cdt.insert_constraint(poly[i], poly[(i+1)%poly.size()]);
			}
		}

		//Mark facets that are inside the domain bounded by the polygon
		mark_domains(cdt);

		// Iterate over the resulting faces
		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
				 fit != cdt.finite_faces_end(); fit++) {
			if (fit->info().in_domain()) {
				Polygon tri;
				for (int i=0;i<3;i++) {
					Vertex3K v = cdt.triangle(fit)[i];
					tri.push_back(Vector3d(v.x(), v.y(), v.z()));
				}
				triangles.push_back(tri);
			}
		}

		return false;
	}

	bool tessellatePolygon(const PolygonK &polygon,
												 Polygons &triangles,
												 const K::Vector_3 *normal)
	{
		if (polygon.size() == 3) {
			PRINTD("input polygon has 3 points. shortcut tessellation.");
			Polygon t;
			t.push_back(Vector3d(polygon[0].x(), polygon[0].y(), polygon[0].z()));
			t.push_back(Vector3d(polygon[1].x(), polygon[1].y(), polygon[1].z()));
			t.push_back(Vector3d(polygon[2].x(), polygon[2].y(), polygon[2].z()));
			triangles.push_back(t);
			return false;
		}

		K::Vector_3 normalvec;
		if (normal) {
			normalvec = *normal;
		}
		else {
			// Calculate best guess at face normal using Newell's method
			CGAL::normal_vector_newell_3(polygon.begin(), polygon.end(), normalvec);
		}
		double sqrl = normalvec.squared_length();
		if (sqrl > 0.0) normalvec = normalvec / sqrt(sqrl);
		
		// Pass the normal vector to the (undocumented)
		// CGAL::Triangulation_2_filtered_projection_traits_3. This
		// trait deals with projection from 3D to 2D using the normal
		// vector as a hint, and allows for near-planar polygons to be passed to
		// the Constrained Delaunay Triangulator.
		Projection actualProjection(normalvec);
		CDT cdt(actualProjection);
		for (size_t i=0;i<polygon.size(); i++) {
			cdt.insert_constraint(polygon[i], polygon[(i+1)%polygon.size()]);
		}
		
		//Mark facets that are inside the domain bounded by the polygon
		mark_domains(cdt);

		// Iterate over the resulting faces
		CDT::Finite_faces_iterator fit;
		for (fit=cdt.finite_faces_begin(); fit!=cdt.finite_faces_end(); fit++) {
			if (fit->info().in_domain()) {
				Polygon tri;
				for (int i=0;i<3;i++) {
					K::Point_3 v = cdt.triangle(fit)[i];
					tri.push_back(Vector3d(v.x(), v.y(), v.z()));
				}
				triangles.push_back(tri);
			}
		}

		return false;
	}

}; // namespace CGALUtils

