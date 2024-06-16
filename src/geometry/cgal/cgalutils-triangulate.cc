// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "PolySetBuilder.h"
#include "cgalutils.h"

#include <utility>

#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/exceptions.h>

namespace CGALUtils {

namespace Polygon2DCGAL {

struct FaceInfo
{
  FaceInfo() = default;
  int nesting_level{42};
  [[nodiscard]] bool in_domain() const { return nesting_level % 2 == 1; }
};


using K = CGAL::Exact_predicates_inexact_constructions_kernel;
using Vb = CGAL::Triangulation_vertex_base_with_info_2<int, K>;
using Fbb = CGAL::Triangulation_face_base_with_info_2<FaceInfo, K>;
using Fb = CGAL::Constrained_triangulation_face_base_2<K, Fbb>;
using TDS = CGAL::Triangulation_data_structure_2<Vb, Fb>;
using Itag = CGAL::Exact_predicates_tag;
using CDT = CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag>;
using Point = CDT::Point;
using Polygon_2 = CGAL::Polygon_2<K>;

void
mark_domains(CDT& ct,
             CDT::Face_handle start,
             int index,
             std::list<CDT::Edge>& border)
{
  if (start->info().nesting_level != -1) return;

  std::list<CDT::Face_handle> queue;
  queue.push_back(start);

  while (!queue.empty()) {
    auto fh = queue.front();
    queue.pop_front();
    if (fh->info().nesting_level == -1) {
      fh->info().nesting_level = index;
      for (int i = 0; i < 3; ++i) {
        CDT::Edge e(fh, i);
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
mark_domains(CDT& cdt)
{
  for (CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it) {
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
      mark_domains(cdt, n, e.first->info().nesting_level + 1, border);
    }
  }
}

} // namespace Polygon2DCGAL

template <typename Polyhedron>
void triangulateFaces(Polyhedron& polyhedron)
{
  CGAL::Polygon_mesh_processing::triangulate_faces(polyhedron);
}

template void triangulateFaces(CGAL_HybridMesh& polyhedron);
template void triangulateFaces(CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>>& polyhedron);

std::unique_ptr<PolySet> createTriangulatedPolySetFromPolygon2d(const Polygon2d& polygon2d)
{
  auto polyset = std::make_unique<PolySet>(2); 
  polyset->setTriangular(true);

  Polygon2DCGAL::CDT cdt; // Uses a constrained Delaunay triangulator.

  try {
    // Adds all vertices, and add all contours as constraints.
    for (const auto& outline : polygon2d.outlines()) {
      // Start with last point
      int last_idx = outline.vertices.size() - 1;
      auto prev = cdt.insert({outline.vertices[last_idx][0], outline.vertices[last_idx][1]});
      prev->info() = last_idx;
      for (int i=0;i<outline.vertices.size();i++) {
        const auto &v = outline.vertices[i];
        polyset->vertices.emplace_back(v[0], v[1], 0.0);
        auto curr = cdt.insert({v[0], v[1]});
        curr->info() = polyset->vertices.size() - 1;
          cdt.insert_constraint(prev, curr);
          prev = curr;
      }
    }

  } catch (const CGAL::Precondition_exception& e) {
    LOG("CGAL error in Polygon2d::tesselate(): %1$s", e.what());
    return nullptr;
  }

  // To extract triangles which is part of our polygon, we need to filter away
  // triangles inside holes.
  mark_domains(cdt);
  for (auto fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) {
    if (fit->info().in_domain()) {
      polyset->indices.push_back({fit->vertex(0)->info(), fit->vertex(1)->info(), fit->vertex(2)->info()});
    }
  }
  return polyset;
}

} // namespace CGALUtils

