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
std::shared_ptr<const Geometry> applyHull(const Geometry::Geometries& children)
{
  return std::make_shared<PolySet>(3, true);
}

std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children)
{
  return std::make_shared<PolySet>(3);
}
#endif  // !ENABLE_CGAL
