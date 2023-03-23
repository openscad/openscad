// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgal.h"
#include "cgalutils.h"
#include "Feature.h"
#include "PolySet.h"
#include "printutils.h"
#include "progress.h"
#include "CGALHybridPolyhedron.h"
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#include "manifoldutils.h"
#endif
#include "node.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>

#include <CGAL/config.h>
#include <CGAL/version.h>

#include <CGAL/convex_hull_3.h>

#include "memory.h"
#include "Reindexer.h"
#include "GeometryUtils.h"

#include <map>
#include <queue>
#include <unordered_set>

namespace CGALUtils {

/*!
   Applies op to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */
shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries& children, OpenSCADOperator op)
{
  if (Feature::ExperimentalFastCsg.is_enabled()) {
    return applyOperator3DHybrid(children, op);
  }

  CGAL_Nef_polyhedron *N = nullptr;

  assert(op != OpenSCADOperator::UNION && "use applyUnion3D() instead of applyOperator3D()");
  bool foundFirst = false;

  try {
    for (const auto& item : children) {
      const shared_ptr<const Geometry>& chgeom = item.second;
      auto chN = getNefPolyhedronFromGeometry(chgeom);

      // Initialize N with first expected geometric object
      if (!foundFirst) {
        if (chN) {
          N = new CGAL_Nef_polyhedron(*chN);
        } else { // first child geometry might be empty/null
          N = nullptr;
        }
        foundFirst = true;
        continue;
      }

      // Intersecting something with nothing results in nothing
      if (!chN || chN->isEmpty()) {
        if (op == OpenSCADOperator::INTERSECTION) {
          if (N != nullptr) delete N; // safety!
          N = nullptr;
        }
        continue;
      }

      // empty op <something> => empty
      if (!N || N->isEmpty()) continue;

      switch (op) {
      case OpenSCADOperator::INTERSECTION:
        *N *= *chN;
        break;
      case OpenSCADOperator::DIFFERENCE:
        *N -= *chN;
        break;
      case OpenSCADOperator::MINKOWSKI:
        N->minkowski(*chN);
        break;
      default:
        LOG(message_group::Error, "Unsupported CGAL operator: %1$d", static_cast<int>(op));
      }
      if (item.first) item.first->progress_report();
    }
  }
  // union && difference assert triggered by tests/data/scad/bugs/rotate-diff-nonmanifold-crash.scad and tests/data/scad/bugs/issue204.scad
  catch (const CGAL::Failure_exception& e) {
    std::string opstr = op == OpenSCADOperator::INTERSECTION ? "intersection" : op == OpenSCADOperator::DIFFERENCE ? "difference" : op == OpenSCADOperator::UNION ? "union" : "UNKNOWN";
    LOG(message_group::Error, "CGAL error in CGALUtils::applyBinaryOperator %1$s: %2$s", opstr, e.what());
  }
  // boost any_cast throws exceptions inside CGAL code, ending here https://github.com/openscad/openscad/issues/3756
  catch (const std::exception& e) {
    std::string opstr = op == OpenSCADOperator::INTERSECTION ? "intersection" : op == OpenSCADOperator::DIFFERENCE ? "difference" : op == OpenSCADOperator::UNION ? "union" : "UNKNOWN";
    LOG(message_group::Error, "exception in CGALUtils::applyBinaryOperator %1$s: %2$s", opstr, e.what());
  }
  return shared_ptr<Geometry>(N);
}

shared_ptr<const Geometry> applyUnion3D(
  Geometry::Geometries::iterator chbegin, Geometry::Geometries::iterator chend)
{
  if (Feature::ExperimentalFastCsg.is_enabled()) {
    return applyUnion3DHybrid(chbegin, chend);
  }

  using QueueConstItem = std::pair<shared_ptr<const CGAL_Nef_polyhedron>, int>;
  struct QueueItemGreater {
    // stable sort for priority_queue by facets, then progress mark
    bool operator()(const QueueConstItem& lhs, const QueueConstItem& rhs) const
    {
      size_t l = lhs.first->p3->number_of_facets();
      size_t r = rhs.first->p3->number_of_facets();
      return (l > r) || (l == r && lhs.second > rhs.second);
    }
  };
  std::priority_queue<QueueConstItem, std::vector<QueueConstItem>, QueueItemGreater> q;

  try {
    // sort children by fewest faces
    for (auto it = chbegin; it != chend; ++it) {
      auto curChild = getNefPolyhedronFromGeometry(it->second);
      if (curChild && !curChild->isEmpty()) {
        int node_mark = -1;
        if (it->first) {
          node_mark = it->first->progress_mark;
        }
        q.emplace(curChild, node_mark);
      }
    }

    progress_tick();
    while (q.size() > 1) {
      auto p1 = q.top();
      q.pop();
      auto p2 = q.top();
      q.pop();
      q.emplace(make_shared<const CGAL_Nef_polyhedron>(*p1.first + *p2.first), -1);
      progress_tick();
    }

    if (q.size() == 1) {
      return shared_ptr<const Geometry>(new CGAL_Nef_polyhedron(q.top().first->p3));
    } else {
      return nullptr;
    }
  } catch (const CGAL::Failure_exception& e) {
    LOG(message_group::Error, "CGAL error in CGALUtils::applyUnion3D: %1$s", e.what());
  }
  return nullptr;
}



bool applyHull(const Geometry::Geometries& children, PolySet& result)
{
  using K = CGAL::Epick;
  // Collect point cloud
  Reindexer<K::Point_3> reindexer;
  std::vector<K::Point_3> points;
  size_t pointsSaved = 0;

  auto addPoint = [&](const auto& v) {
      size_t s = reindexer.size();
      size_t idx = reindexer.lookup(v);
      if (idx == s) {
        points.push_back(vector_convert<K::Point_3>(v));
      } else {
        pointsSaved++;
      }
    };

  for (const auto& item : children) {
    auto& chgeom = item.second;
    if (auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom)) {
      if (!N->isEmpty()) {
        points.reserve(points.size() + N->p3->number_of_vertices());
        for (CGAL_Nef_polyhedron3::Vertex_const_iterator i = N->p3->vertices_begin(); i != N->p3->vertices_end(); ++i) {
          addPoint(vector_convert<K::Point_3>(i->point()));
        }
      }
    } else if (auto hybrid = dynamic_pointer_cast<const CGALHybridPolyhedron>(chgeom)) {
      points.reserve(points.size() + hybrid->numVertices());
      hybrid->foreachVertexUntilTrue([&](auto& p) {
          addPoint(vector_convert<K::Point_3>(p));
          return false;
        });
#ifdef ENABLE_MANIFOLD
    } else if (auto mani = dynamic_pointer_cast<const ManifoldGeometry>(chgeom)) {
      points.reserve(points.size() + mani->numVertices());
      mani->foreachVertexUntilTrue([&](auto& p) {
          addPoint(vector_convert<K::Point_3>(p));
          return false;
        });
#endif
    } else {
      const auto *ps = dynamic_cast<const PolySet *>(chgeom.get());
      if (ps) {
        points.reserve(points.size() + ps->polygons.size() * 3);
        for (const auto& p : ps->polygons) {
          for (const auto& v : p) {
            addPoint(vector_convert<K::Point_3>(v));
          }
        }
      }
    }
  }

  if (points.size() <= 3) return false;

  // Apply hull
  bool success = false;
  if (points.size() >= 4) {
    try {
      CGAL::Polyhedron_3<K> r;
      CGAL::convex_hull_3(points.begin(), points.end(), r);
      PRINTDB("After hull vertices: %d", r.size_of_vertices());
      PRINTDB("After hull facets: %d", r.size_of_facets());
      PRINTDB("After hull closed: %d", r.is_closed());
      PRINTDB("After hull valid: %d", r.is_valid());
      success = !createPolySetFromPolyhedron(r, result);
    } catch (const CGAL::Failure_exception& e) {
      LOG(message_group::Error, "CGAL error in applyHull(): %1$s", e.what());
    }
  }
  return success;
}


/*!
   children cannot contain nullptr objects
 */
shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children)
{
#if ENABLE_MANIFOLD
  if (Feature::ExperimentalManifold.is_enabled()) {
    return ManifoldUtils::applyMinkowskiManifold(children);
  }
#endif
  if (Feature::ExperimentalFastCsg.is_enabled()) {
    return applyMinkowskiHybrid(children);
  }
  CGAL::Timer t, t_tot;
  assert(children.size() >= 2);
  auto it = children.begin();
  t_tot.start();
  shared_ptr<const Geometry> operands[2] = {it->second, shared_ptr<const Geometry>()};
  try {
    while (++it != children.end()) {
      operands[1] = it->second;

      using Hull_kernel = CGAL::Epick;

      std::list<CGAL_Polyhedron> P[2];
      std::list<CGAL::Polyhedron_3<Hull_kernel>> result_parts;

      for (size_t i = 0; i < 2; ++i) {
        CGAL_Polyhedron poly;

        auto ps = dynamic_pointer_cast<const PolySet>(operands[i]);
        auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(operands[i]);

        if (!nef) {
          nef = CGALUtils::getNefPolyhedronFromGeometry(operands[i]);
        }

        if (ps) CGALUtils::createPolyhedronFromPolySet(*ps, poly);
        else if (nef && nef->p3->is_simple()) CGALUtils::convertNefToPolyhedron(*nef->p3, poly);
        else throw 0;

        if ((ps && ps->is_convex()) ||
            (!ps && is_weakly_convex(poly))) {
          PRINTDB("Minkowski: child %d is convex and %s", i % (ps?"PolySet":"Nef"));
          P[i].push_back(poly);
        } else {
          CGAL_Nef_polyhedron3 decomposed_nef;

          if (ps) {
            PRINTDB("Minkowski: child %d is nonconvex PolySet, transforming to Nef and decomposing...", i);
            auto p = getNefPolyhedronFromGeometry(ps);
            if (p && !p->isEmpty()) decomposed_nef = *p->p3;
          } else {
            PRINTDB("Minkowski: child %d is nonconvex Nef, decomposing...", i);
            decomposed_nef = *nef->p3;
          }

          t.start();
          CGAL::convex_decomposition_3(decomposed_nef);

          // the first volume is the outer volume, which ignored in the decomposition
          CGAL_Nef_polyhedron3::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
          for (; ci != decomposed_nef.volumes_end(); ++ci) {
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
            std::advance(it, k == 0?i:j);

            CGAL_Polyhedron const& poly = *it;
            points[k].reserve(poly.size_of_vertices());

            for (CGAL_Polyhedron::Vertex_const_iterator pi = poly.vertices_begin(); pi != poly.vertices_end(); ++pi) {
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
          PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points[0].size() % points[1].size() % minkowski_points.size() % (t.time() * 1000));
          t.reset();

          t.start();

          CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), result);

          std::vector<Hull_kernel::Point_3> strict_points;
          strict_points.reserve(minkowski_points.size());

          for (CGAL::Polyhedron_3<Hull_kernel>::Vertex_iterator i = result.vertices_begin(); i != result.vertices_end(); ++i) {
            Hull_kernel::Point_3 const& p = i->point();

            CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle h, e;
            h = i->halfedge();
            e = h;
            bool collinear = false;
            bool coplanar = true;

            do {
              Hull_kernel::Point_3 const& q = h->opposite()->vertex()->point();
              if (coplanar && !CGAL::coplanar(p, q,
                                              h->next_on_vertex()->opposite()->vertex()->point(),
                                              h->next_on_vertex()->next_on_vertex()->opposite()->vertex()->point())) {
                coplanar = false;
              }


              for (CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle j = h->next_on_vertex();
                   j != h && !collinear && !coplanar;
                   j = j->next_on_vertex()) {

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

      auto partToGeom = [&](auto& poly) -> shared_ptr<const Geometry> {
          auto *ps = new PolySet(3, /* convex= */ true);
          createPolySetFromPolyhedron(poly, *ps);
          return shared_ptr<const Geometry>(ps);
        };

      if (result_parts.size() == 1) {
        operands[0] = partToGeom(*result_parts.begin());
      } else if (!result_parts.empty()) {
        t.start();
        PRINTDB("Minkowski: Computing union of %d parts", result_parts.size());
        Geometry::Geometries fake_children;
        for (const auto& part : result_parts) {
          fake_children.push_back(std::make_pair(std::shared_ptr<const AbstractNode>(),
                                                 partToGeom(part)));
        }
        auto N = CGALUtils::applyUnion3D(fake_children.begin(), fake_children.end());
        // FIXME: This should really never throw.
        // Assert once we figured out what went wrong with issue #1069?
        if (!N) throw 0;
        t.stop();
        PRINTDB("Minkowski: Union done: %f s", t.time());
        t.reset();
        operands[0] = N;
      } else {
        operands[0] = shared_ptr<const Geometry>(new CGAL_Nef_polyhedron());
      }
    }

    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  } catch (...) {
    // If anything throws we simply fall back to Nef Minkowski
    PRINTD("Minkowski: Falling back to Nef Minkowski");

    auto N = shared_ptr<const Geometry>(applyOperator3D(children, OpenSCADOperator::MINKOWSKI));
    return N;
  }
}
}  // namespace CGALUtils


#endif // ENABLE_CGAL








