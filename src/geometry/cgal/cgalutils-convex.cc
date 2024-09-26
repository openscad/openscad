#ifdef ENABLE_CGAL

#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include <CGAL/Plane_3.h>
#include <CGAL/Surface_mesh.h>
#include <queue>
#include <unordered_set>

namespace CGALUtils {

template <typename K>
bool is_weakly_convex(const CGAL::Polyhedron_3<K>& p) {
  using Polyhedron = typename CGAL::Polyhedron_3<K>;

  for (typename Polyhedron::Edge_const_iterator i = p.edges_begin(); i != p.edges_end(); ++i) {
    typename Polyhedron::Plane_3 p(i->opposite()->vertex()->point(), i->vertex()->point(), i->next()->vertex()->point());
    if (p.has_on_positive_side(i->opposite()->next()->vertex()->point()) &&
        CGAL::squared_distance(p, i->opposite()->next()->vertex()->point()) > 1e-8) {
      return false;
    }
  }
  // Also make sure that there is only one shell:
  std::unordered_set<typename Polyhedron::Facet_const_handle, typename CGAL::Handle_hash_function> visited;
  // c++11
  visited.reserve(p.size_of_facets());

  std::queue<typename Polyhedron::Facet_const_handle> to_explore;
  to_explore.push(p.facets_begin()); // One arbitrary facet
  visited.insert(to_explore.front());

  while (!to_explore.empty()) {
    typename Polyhedron::Facet_const_handle f = to_explore.front();
    to_explore.pop();
    typename Polyhedron::Facet::Halfedge_around_facet_const_circulator he, end;
    end = he = f->facet_begin();
    CGAL_For_all(he, end) {
      typename Polyhedron::Facet_const_handle o = he->opposite()->facet();

      if (!visited.count(o)) {
        visited.insert(o);
        to_explore.push(o);
      }
    }
  }

  return visited.size() == p.size_of_facets();
}

template bool is_weakly_convex(const CGAL::Polyhedron_3<CGAL_Kernel3>& p);


template <typename K>
bool is_weakly_convex(const CGAL::Surface_mesh<CGAL::Point_3<K>>& m) {
  using Mesh = typename CGAL::Surface_mesh<CGAL::Point_3<K>>;

  for (auto i : m.halfedges()) {
    CGAL::Plane_3<K> p(
      m.point(m.target(m.opposite(i))),
      m.point(m.target(i)),
      m.point(m.target(m.next(i))));
    const auto& pt = m.point(m.target(m.next(m.opposite(i))));
    if (p.has_on_positive_side(pt) && CGAL::squared_distance(p, pt) > 1e-8) {
      return false;
    }
  }

  // Also make sure that there is only one shell:
  std::unordered_set<typename Mesh::Face_index, typename CGAL::Handle_hash_function> visited;
  visited.reserve(m.number_of_faces());

  std::queue<typename Mesh::Face_index> to_explore;
  to_explore.push(*m.faces().begin()); // One arbitrary facet
  visited.insert(to_explore.front());

  while (!to_explore.empty()) {
    typename Mesh::Face_index f = to_explore.front();
    to_explore.pop();

    CGAL::Halfedge_around_face_iterator<Mesh> he, end;
    for (boost::tie(he, end) = CGAL::halfedges_around_face(m.halfedge(f), m); he != end; ++he) {
      typename Mesh::Face_index o = m.face(m.opposite(*he));

      if (!visited.count(o)) {
        visited.insert(o);
        to_explore.push(o);
      }
    }
  }

  return visited.size() == m.number_of_faces();
}

template bool is_weakly_convex(const CGAL::Polyhedron_3<CGAL_HybridKernel3>& p);
template bool is_weakly_convex(const CGAL_HybridMesh& p);

}  // namespace CGALUtils

#endif // ENABLE_CGAL
