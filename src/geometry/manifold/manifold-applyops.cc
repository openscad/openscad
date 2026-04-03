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
    case OpenSCADOperator::UNION:        *geom = *geom + *chN; break;
    case OpenSCADOperator::INTERSECTION: *geom = *geom * *chN; break;
    case OpenSCADOperator::DIFFERENCE:   *geom = *geom - *chN; break;
    case OpenSCADOperator::MINKOWSKI:    *geom = geom->minkowski(*chN); break;
    default:                             LOG(message_group::Error, "Unsupported CGAL operator: %1$d", static_cast<int>(op));
    }
    if (item.first) item.first->progress_report();
  }
  return geom;
}

};  // namespace ManifoldUtils

#endif  // ENABLE_MANIFOLD
