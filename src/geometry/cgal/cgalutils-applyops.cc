// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM
#include "geometry/cgal/cgal.h"
#include "geometry/Geometry.h"
#include "geometry/cgal/cgalutils.h"
#include "Feature.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"
#include "core/progress.h"
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#include "geometry/manifold/manifoldutils.h"
#endif
#include "core/node.h"

#include <cassert>
#include <utility>
#include <exception>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>

#include <CGAL/config.h>
#include <CGAL/version.h>

#include <CGAL/convex_hull_3.h>

#include "geometry/Reindexer.h"
#include "geometry/GeometryUtils.h"

#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

namespace CGALUtils {

namespace {

const char *opToString(OpenSCADOperator op)
{
  switch (op) {
  case OpenSCADOperator::UNION:        return "union";
  case OpenSCADOperator::INTERSECTION: return "intersection";
  case OpenSCADOperator::DIFFERENCE:   return "difference";
  case OpenSCADOperator::MINKOWSKI:    return "minkowski";
  case OpenSCADOperator::HULL:         return "hull";
  case OpenSCADOperator::FILL:         return "fill";
  case OpenSCADOperator::RESIZE:       return "resize";
  case OpenSCADOperator::OFFSET:       return "offset";
  }
  return "UNKNOWN";
}

}  // namespace

std::unique_ptr<const Geometry> applyUnion3D(const CsgOpNode& node,
                                             Geometry::Geometries::iterator chbegin,
                                             Geometry::Geometries::iterator chend)
{
  using QueueConstItem = std::pair<std::shared_ptr<const CGALNefGeometry>, int>;
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
      q.emplace(std::make_unique<const CGALNefGeometry>(*p1.first + *p2.first), -1);
      progress_tick();
    }

    if (q.size() == 1) {
      return std::make_unique<CGALNefGeometry>(q.top().first->p3);
    } else {
      return nullptr;
    }
  } catch (const CGAL::Failure_exception& e) {
    LOG(message_group::Error, "CGAL error in CGALUtils::applyUnion3D: %1$s", e.what());
  }
  return nullptr;
}

/*!
   Applies op to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */

std::unique_ptr<const Geometry> addFillets(std::shared_ptr<const Geometry> result,
                                           const Geometry::Geometries& children, double r, int fn);

std::shared_ptr<const Geometry> applyOperator3D(const CsgOpNode& node,
                                                const Geometry::Geometries& children,
                                                OpenSCADOperator op)
{
  std::shared_ptr<CGALNefGeometry> N;

  assert(op != OpenSCADOperator::UNION && "use applyUnion3D() instead of applyOperator3D()");
  bool foundFirst = false;

  try {
    for (const auto& item : children) {
      const std::shared_ptr<const Geometry>& chgeom = item.second;
      auto chN = getNefPolyhedronFromGeometry(chgeom);

      // Initialize N with first expected geometric object
      if (!foundFirst) {
        if (chN) {
          // FIXME: Do we need to make a copy here?
          N = std::make_shared<CGALNefGeometry>(*chN);
        } else {  // first child geometry might be empty/null
          N = nullptr;
        }
        foundFirst = true;
        continue;
      }

      // Intersecting something with nothing results in nothing
      if (!chN || chN->isEmpty()) {
        if (op == OpenSCADOperator::INTERSECTION) {
          N = nullptr;
        }
        continue;
      }

      // empty op <something> => empty
      if (!N || N->isEmpty()) continue;

      switch (op) {
      case OpenSCADOperator::INTERSECTION: *N *= *chN; break;
      case OpenSCADOperator::DIFFERENCE:   *N -= *chN; break;
      case OpenSCADOperator::MINKOWSKI:
#if defined(_MSC_VER)
      {
        // On MSVC, CGAL Nef minkowski has been observed to crash with access violations
        // for large inputs. Avoid calling into the fragile code for very large inputs
        // and instead fall back to a safe (empty) result so tests fail gracefully
        // instead of crashing the whole test runner.
        size_t f0 = (N && N->p3) ? N->p3->number_of_facets() : 0;
        size_t f1 = (chN && chN->p3) ? chN->p3->number_of_facets() : 0;
        const size_t FACET_LIMIT = 20000;
        if (f0 > FACET_LIMIT || f1 > FACET_LIMIT) {
          LOG(message_group::Warning,
              "Skipping CGAL Nef minkowski on MSVC due to large input (%1$d, %2$d facets)", f0, f1);
          N = nullptr;
          break;
        }
      }
#endif
        N->minkowski(*chN);
        break;
      default: LOG(message_group::Error, "Unsupported CGAL operator: %1$d", static_cast<int>(op));
      }
      if (item.first) item.first->progress_report();
    }
  }
  // union && difference assert triggered by tests/data/scad/bugs/rotate-diff-nonmanifold-crash.scad and
  // tests/data/scad/bugs/issue204.scad
  catch (const CGAL::Failure_exception& e) {
    LOG(message_group::Error, "CGAL error in CGALUtils::applyOperator3D %1$s: %2$s", opToString(op),
        e.what());
  }
  // boost any_cast throws exceptions inside CGAL code, ending here
  // https://github.com/openscad/openscad/issues/3756
  catch (const std::exception& e) {
    LOG(message_group::Error, "exception in CGALUtils::applyOperator3D %1$s: %2$s", opToString(op),
        e.what());
  }
  //
  if (node.r != 0) {
    //    std::unique_ptr<const Geometry> geom_u = addFillets(N, children, node.r, node.fn);
    //    std::shared_ptr<const Geometry> geom_s(geom_u.release());
    //    N=geom_s; //  = ManifoldUtils::createManifoldFromGeometry(geom_s);
  }

  return N;
}

}  // namespace CGALUtils
