// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_MANIFOLD

#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "node.h"
#include "progress.h"
#include "printutils.h"

namespace ManifoldUtils {

Location getLocation(const std::shared_ptr<const AbstractNode>& node)
{
  return node && node->modinst ? node->modinst->location() : Location::NONE;
}

std::shared_ptr<ManifoldGeometry> applyOperator(const Geometry::Geometries& children, OpenSCADOperator op)
{
  manifold::OpType manifold_op;
  switch(op) {
    case OpenSCADOperator::UNION:
    manifold_op = manifold::OpType::Add;
    break;
    case OpenSCADOperator::INTERSECTION:
    manifold_op = manifold::OpType::Intersect;
    break;
    case OpenSCADOperator::DIFFERENCE:
    manifold_op = manifold::OpType::Subtract;
    break;
    default:
    PRINTDB("Unexpected op %1$d", static_cast<int>(op));
    assert(false && "Unexpected op");
  }

  std::vector<manifold::Manifold> manifold_children;
  for (const auto& item : children) {
    manifold_children.push_back(item.second ? createManifoldFromGeometry(item.second)->getManifold() : manifold::Manifold());
   if (item.first) item.first->progress_report();
  }
  auto result = manifold::Manifold::BatchBoolean(manifold_children, manifold_op);
  return std::make_shared<ManifoldGeometry>(std::make_shared<manifold::Manifold>(result));
}

/*!
   Applies minkowski to all children and returns the result.
   The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
 */
std::shared_ptr<ManifoldGeometry> applyMinkowski(const Geometry::Geometries& children)
{
  std::shared_ptr<ManifoldGeometry> geom;

  for (const auto& item : children) {
    auto child_geom = item.second ? createManifoldFromGeometry(item.second) : nullptr;
    if (!child_geom || child_geom->isEmpty()) continue;

    if (!geom) {
      geom = std::make_shared<ManifoldGeometry>(*child_geom);
    } else {
      *geom = geom->minkowski(*child_geom);
    }
    if (item.first) item.first->progress_report();
  }
  return geom;
}

};  // namespace ManifoldUtils

#endif // ENABLE_MANIFOLD
