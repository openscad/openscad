#include "geometry/boolean_utils.h"

#include <memory>
#include <utility>
#include <vector>

#ifdef ENABLE_CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_3.h>

#include "geometry/cgal/CGALNefGeometry.h"
#include "geometry/cgal/cgalutils.h"
#endif  // ENABLE_CGAL
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#include "geometry/manifold/manifoldutils.h"
#endif  // ENABLE_MANIFOLD

#include "geometry/GeometryUtils.h"
#include "geometry/PolySet.h"
#include "geometry/Reindexer.h"
#include "glview/RenderSettings.h"
#include "utils/printutils.h"

#ifdef ENABLE_CGAL
std::unique_ptr<PolySet> applyHull(const Geometry::Geometries& children)
{
  using Hull_kernel = CGAL::Epick;
  // Collect point cloud
  Reindexer<Hull_kernel::Point_3> reindexer;

  auto addCapacity = [&](const auto n) { reindexer.reserve(reindexer.size() + n); };

  auto addPoint = [&](const auto& v) { reindexer.lookup(v); };

  for (const auto& item : children) {
    auto& chgeom = item.second;
#ifdef ENABLE_CGAL
    if (const auto *N = dynamic_cast<const CGALNefGeometry *>(chgeom.get())) {
      if (!N->isEmpty()) {
        addCapacity(N->p3->number_of_vertices());
        for (auto it = N->p3->vertices_begin(); it != N->p3->vertices_end(); ++it) {
          addPoint(CGALUtils::vector_convert<Hull_kernel::Point_3>(it->point()));
        }
      }
#endif  // ENABLE_CGAL
#ifdef ENABLE_MANIFOLD
    } else if (const auto *mani = dynamic_cast<const ManifoldGeometry *>(chgeom.get())) {
      addCapacity(mani->numVertices());
      mani->foreachVertexUntilTrue([&](auto& p) {
        addPoint(CGALUtils::vector_convert<Hull_kernel::Point_3>(p));
        return false;
      });
#endif  // ENABLE_MANIFOLD
    } else if (const auto *ps = dynamic_cast<const PolySet *>(chgeom.get())) {
      addCapacity(ps->indices.size() * 3);
      for (const auto& p : ps->indices) {
        for (const auto& ind : p) {
          addPoint(CGALUtils::vector_convert<Hull_kernel::Point_3>(ps->vertices[ind]));
        }
      }
    }
  }

  const auto& points = reindexer.getArray();
  if (points.size() <= 3) return nullptr;

  // Apply hull
  if (points.size() >= 4) {
    try {
      CGAL::Polyhedron_3<Hull_kernel> r;
      CGAL::convex_hull_3(points.begin(), points.end(), r);
      PRINTDB("After hull vertices: %d", r.size_of_vertices());
      PRINTDB("After hull facets: %d", r.size_of_facets());
      PRINTDB("After hull closed: %d", r.is_closed());
      PRINTDB("After hull valid: %d", r.is_valid());
      // FIXME: Make sure PolySet is set to convex.
      // FIXME: Can we guarantee a manifold PolySet here?
      return CGALUtils::createPolySetFromPolyhedron(r);
    } catch (const CGAL::Failure_exception& e) {
      LOG(message_group::Error, "CGAL error in applyHull(): %1$s", e.what());
    }
  }
  return nullptr;
}

/*!
   children cannot contain nullptr objects

  FIXME: This shouldn't return const, but it does due to internal implementation details
 */
std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children)
{
#if ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
#if defined(USE_MANIFOLD_MINKOWSKI)
    return ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::MINKOWSKI);
#else
    return ManifoldUtils::applyMinkowski(children);
#endif
  }
#endif  // ENABLE_MANIFOLD
  return CGALUtils::applyMinkowski3D(children);
}
#else   // ENABLE_CGAL
std::unique_ptr<PolySet> applyHull(const Geometry::Geometries& children)
{
  return std::make_unique<PolySet>(3, true);
}

std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children)
{
  return std::make_shared<PolySet>(3);
}
#endif  // !ENABLE_CGAL
