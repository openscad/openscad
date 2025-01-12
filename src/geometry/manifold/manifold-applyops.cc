// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_MANIFOLD

#include <memory>
#include "geometry/manifold/manifoldutils.h"
#include "geometry/manifold/ManifoldGeometry.h"
#include "core/node.h"
#include "core/progress.h"
#include "utils/printutils.h"

namespace ManifoldUtils {

Location getLocation(const std::shared_ptr<const AbstractNode>& node)
{
  return node && node->modinst ? node->modinst->location() : Location::NONE;
}

/*!
   Applies op to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */
std::shared_ptr<ManifoldGeometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op)
{
  std::shared_ptr<ManifoldGeometry> geom;

  bool foundFirst = false;

  for (const auto& item : children) {
    auto chN = item.second ? createManifoldFromGeometry(item.second) : nullptr;

    // Intersecting something with nothing results in nothing
    if (!chN || chN->isEmpty()) {
      if (op == OpenSCADOperator::INTERSECTION) {
        geom = nullptr;
        break;
      }
      if (op == OpenSCADOperator::DIFFERENCE && !foundFirst) {
        geom = nullptr;
        break;
      }
      continue;
    }

    // Initialize geom with first expected geometric object
    if (!foundFirst) {
      geom = std::make_shared<ManifoldGeometry>(*chN);
      foundFirst = true;
      continue;
    }

    switch (op) {
    case OpenSCADOperator::UNION:
      *geom = *geom + *chN;
      break;
    case OpenSCADOperator::INTERSECTION:
      *geom = *geom * *chN;
      break;
    case OpenSCADOperator::DIFFERENCE:
      *geom = *geom - *chN;
      break;
    case OpenSCADOperator::MINKOWSKI:
      *geom = geom->minkowski(*chN);
      break;
    default:
      LOG(message_group::Error, "Unsupported CGAL operator: %1$d", static_cast<int>(op));
    }
    if (item.first) item.first->progress_report();
  }
  return geom;
}

};  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
