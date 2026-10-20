// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_MANIFOLD

#include <memory>

#include "core/AST.h"
#include "core/enums.h"
#include "core/node.h"
#include "core/progress.h"
#include "geometry/Geometry.h"
#include "geometry/PolySet.h"
#include "geometry/manifold/ManifoldGeometry.h"
#include "geometry/manifold/manifoldutils.h"
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
std::shared_ptr<ManifoldGeometry> applyOperator3DManifold(const Geometry::Geometries& children,
                                                          OpenSCADOperator op)
{
  if (op == OpenSCADOperator::HULL) {
    std::vector<manifold::vec3> pts;
    for (const auto& item : children) {
      if (!item.second) continue;
      auto& chgeom = item.second;
      if (const auto *mani = dynamic_cast<const ManifoldGeometry *>(chgeom.get())) {
        pts.reserve(pts.size() + mani->numVertices());
        mani->foreachVertexUntilTrue([&](auto& p) {
          pts.push_back(p);
          return false;
        });
      } else if (const auto *ps = dynamic_cast<const PolySet *>(chgeom.get())) {
        pts.reserve(pts.size() + ps->indices.size() * 3);
        for (const auto& p : ps->indices) {
          for (const auto& ind : p) {
            auto& v = ps->vertices[ind];
            pts.push_back({v[0], v[1], v[2]});
          }
        }
      } else {
        auto chN = createManifoldFromGeometry(chgeom);
        if (chN && !chN->isEmpty()) {
          chN->foreachVertexUntilTrue([&](auto& p) {
            pts.push_back(p);
            return false;
          });
        }
      }
      if (item.first) item.first->progress_report();
    }
    if (pts.empty()) return nullptr;
    return std::make_shared<ManifoldGeometry>(manifold::Manifold::Hull(pts));
  }

  if (op != OpenSCADOperator::MINKOWSKI) {
    // UNION / INTERSECTION / DIFFERENCE: one batch boolean over all non-empty children
    // (equivalent to the old N-1 sequential left fold, but a single Manifold::BatchBoolean).
    std::vector<std::shared_ptr<const ManifoldGeometry>> mani_children;
    for (const auto& item : children) {
      auto chN = item.second ? createManifoldFromGeometry(item.second) : nullptr;
      if (item.first) item.first->progress_report();
      if (!chN || chN->isEmpty()) {
        // Intersecting something with nothing results in nothing
        if (op == OpenSCADOperator::INTERSECTION) return nullptr;
        if (op == OpenSCADOperator::DIFFERENCE && mani_children.empty()) return nullptr;
        continue;
      }
      mani_children.push_back(chN);
    }
    if (mani_children.empty()) return nullptr;
    if (mani_children.size() == 1) return std::make_shared<ManifoldGeometry>(*mani_children.front());
    manifold::OpType opType;
    switch (op) {
    case OpenSCADOperator::UNION:        opType = manifold::OpType::Add; break;
    case OpenSCADOperator::INTERSECTION: opType = manifold::OpType::Intersect; break;
    case OpenSCADOperator::DIFFERENCE:   opType = manifold::OpType::Subtract; break;
    default:
      LOG(message_group::Error, "Unsupported manifold operator: %1$d", static_cast<int>(op));
      return nullptr;
    }
    return std::make_shared<ManifoldGeometry>(ManifoldGeometry::batchOp(mani_children, opType));
  }

  // MINKOWSKI stays a left fold (no batch variant; the operation is not associative).
  std::shared_ptr<ManifoldGeometry> geom;
  bool foundFirst = false;
  for (const auto& item : children) {
    auto chN = item.second ? createManifoldFromGeometry(item.second) : nullptr;
    if (!chN || chN->isEmpty()) continue;
    // Initialize geom with first expected geometric object
    if (!foundFirst) {
      geom = std::make_shared<ManifoldGeometry>(*chN);
      foundFirst = true;
      continue;
    }
    *geom = geom->minkowski(*chN);
    if (item.first) item.first->progress_report();
  }
  return geom;
}

};  // namespace ManifoldUtils

#endif  // ENABLE_MANIFOLD
