#include "geometry/cgal/cgalutils.h"

#include <memory>

#include <CGAL/Timer.h>
#include <CGAL/convex_hull_3.h>

#include "utils/printutils.h"

namespace CGALUtils {

std::shared_ptr<const Geometry> applyMinkowski3D(const Geometry::Geometries& children)
{
  assert(children.size() >= 2);

  CGAL::Timer t;
  CGAL::Timer t_tot;
  t_tot.start();

  auto it = children.begin();
  std::shared_ptr<const Geometry> operands[2] = {it->second, std::shared_ptr<const Geometry>()};
  try {
    while (++it != children.end()) {
      operands[1] = it->second;

      using Hull_kernel = CGAL::Epick;

      std::list<CGAL_Polyhedron> P[2];
      std::list<CGAL::Polyhedron_3<Hull_kernel>> result_parts;

      for (size_t i = 0; i < 2; ++i) {
        CGAL_Polyhedron poly;

        auto ps = std::dynamic_pointer_cast<const PolySet>(operands[i]);
        auto nef = std::dynamic_pointer_cast<const CGALNefGeometry>(operands[i]);

        if (!nef) {
          nef = CGALUtils::getNefPolyhedronFromGeometry(operands[i]);
        }

        if (ps) CGALUtils::createPolyhedronFromPolySet(*ps, poly);
        else if (nef && nef->p3->is_simple()) CGALUtils::convertNefToPolyhedron(*nef->p3, poly);
        else throw 0;

        if ((ps && ps->isConvex()) || (!ps && CGALUtils::is_weakly_convex(poly))) {
          PRINTDB("Minkowski: child %d is convex and %s", i % (ps ? "PolySet" : "Nef"));
          P[i].push_back(poly);
        } else {
          CGAL_Nef_polyhedron3 decomposed_nef;

          if (ps) {
            PRINTDB("Minkowski: child %d is nonconvex PolySet, transforming to Nef and decomposing...",
                    i);
            auto p = CGALUtils::getNefPolyhedronFromGeometry(ps);
            if (p && !p->isEmpty()) decomposed_nef = *p->p3;
          } else {
            PRINTDB("Minkowski: child %d is nonconvex Nef, decomposing...", i);
            decomposed_nef = *nef->p3;
          }

          t.start();
          CGAL::convex_decomposition_3(decomposed_nef);

          // the first volume is the outer volume, which ignored in the decomposition
          for (auto ci = ++decomposed_nef.volumes_begin(); ci != decomposed_nef.volumes_end(); ++ci) {
            if (ci->mark()) {
              CGAL_Polyhedron poly;
              decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
              P[i].push_back(poly);
            }
          }

          PRINTDB("Minkowski: decomposed into %d convex parts", P[i].size());
          t.stop();
          PRINTDB("Minkowski: decomposition took %f s", t.time());
        }
      }

      std::vector<Hull_kernel::Point_3> points[2];
      std::vector<Hull_kernel::Point_3> minkowski_points;

      CGAL::Cartesian_converter<CGAL_Kernel3, Hull_kernel> conv;

      for (size_t i = 0; i < P[0].size(); ++i) {
        for (size_t j = 0; j < P[1].size(); ++j) {
          t.start();
          points[0].clear();
          points[1].clear();

          for (int k = 0; k < 2; ++k) {
            auto it = P[k].begin();
            std::advance(it, k == 0 ? i : j);

            CGAL_Polyhedron const& poly = *it;
            points[k].reserve(poly.size_of_vertices());

            for (CGAL_Polyhedron::Vertex_const_iterator pi = poly.vertices_begin();
                 pi != poly.vertices_end(); ++pi) {
              CGAL_Polyhedron::Point_3 const& p = pi->point();
              points[k].push_back(conv(p));
            }
          }

          minkowski_points.clear();
          minkowski_points.reserve(points[0].size() * points[1].size());
          for (size_t i = 0; i < points[0].size(); ++i) {
            for (size_t j = 0; j < points[1].size(); ++j) {
              minkowski_points.push_back(points[0][i] + (points[1][j] - CGAL::ORIGIN));
            }
          }

          if (minkowski_points.size() <= 3) {
            t.stop();
            continue;
          }

          CGAL::Polyhedron_3<Hull_kernel> result;
          t.stop();
          PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms",
                  points[0].size() % points[1].size() % minkowski_points.size() % (t.time() * 1000));
          t.reset();

          t.start();

          CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), result);

          std::vector<Hull_kernel::Point_3> strict_points;
          strict_points.reserve(minkowski_points.size());

          for (CGAL::Polyhedron_3<Hull_kernel>::Vertex_iterator i = result.vertices_begin();
               i != result.vertices_end(); ++i) {
            Hull_kernel::Point_3 const& p = i->point();

            CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle h, e;
            h = i->halfedge();
            e = h;
            bool collinear = false;
            bool coplanar = true;

            do {
              Hull_kernel::Point_3 const& q = h->opposite()->vertex()->point();
              if (coplanar && !CGAL::coplanar(
                                p, q, h->next_on_vertex()->opposite()->vertex()->point(),
                                h->next_on_vertex()->next_on_vertex()->opposite()->vertex()->point())) {
                coplanar = false;
              }

              for (CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle j = h->next_on_vertex();
                   j != h && !collinear && !coplanar; j = j->next_on_vertex()) {
                Hull_kernel::Point_3 const& r = j->opposite()->vertex()->point();
                if (CGAL::collinear(p, q, r)) {
                  collinear = true;
                }
              }

              h = h->next_on_vertex();
            } while (h != e && !collinear);

            if (!collinear && !coplanar) strict_points.push_back(p);
          }

          result.clear();
          CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), result);

          t.stop();
          PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
          t.reset();

          result_parts.push_back(result);
        }
      }

      if (it != std::next(children.begin())) operands[0].reset();

      auto partToGeom = [&](auto& poly) -> std::shared_ptr<const Geometry> {
        return CGALUtils::createPolySetFromPolyhedron(poly);
      };

      if (result_parts.size() == 1) {
        operands[0] = partToGeom(*result_parts.begin());
      } else if (!result_parts.empty()) {
        t.start();
        PRINTDB("Minkowski: Computing union of %d parts", result_parts.size());
        Geometry::Geometries fake_children;
        for (const auto& part : result_parts) {
          fake_children.emplace_back(std::shared_ptr<const AbstractNode>(), partToGeom(part));
        }
        auto N = CGALUtils::applyUnion3D(fake_children.begin(), fake_children.end());
        // FIXME: This should really never throw.
        // Assert once we figured out what went wrong with issue #1069?
        if (!N) throw 0;
        t.stop();
        PRINTDB("Minkowski: Union done: %f s", t.time());
        t.reset();
        operands[0] = std::move(N);
      } else {
        operands[0] = std::make_shared<CGALNefGeometry>();
      }
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (...) {
    // If anything throws we simply fall back to Nef Minkowski
    PRINTD("Minkowski: Falling back to Nef Minkowski");

    auto N =
      std::shared_ptr<const Geometry>(CGALUtils::applyOperator3D(children, OpenSCADOperator::MINKOWSKI));
    return N;
  }
}

}  // namespace CGALUtils