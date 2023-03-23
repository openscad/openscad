// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_MANIFOLD

#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "node.h"
#include "progress.h"
#include "printutils.h"

#include <queue>

namespace ManifoldUtils {

Location getLocation(const std::shared_ptr<const AbstractNode>& node)
{
  return node && node->modinst ? node->modinst->location() : Location::NONE;
}

/*!
   Applies op to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */
shared_ptr<const ManifoldGeometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op)
{
  auto N = make_shared<ManifoldGeometry>();

  bool foundFirst = false;

  for (const auto& item : children) {
    auto chN = item.second ? createMutableManifoldFromGeometry(item.second) : nullptr;

    // Intersecting something with nothing results in nothing
    if (!chN || chN->isEmpty()) {
      if (op == OpenSCADOperator::INTERSECTION) {
        N = nullptr;
        break;
      }
      if (op == OpenSCADOperator::DIFFERENCE && !foundFirst) {
        N = nullptr;
        break;
      }
      continue;
    }

    // Initialize N with first expected geometric object
    if (!foundFirst) {
      N = chN;
      foundFirst = true;
      continue;
    }

    switch (op) {
    case OpenSCADOperator::UNION:
      *N += *chN;
      break;
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
  return N;
}

};  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
