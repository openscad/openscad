#pragma once

#include "Geometry.h"
#include "enums.h"
#include "ManifoldGeometry.h"
#include "manifold.h"

class PolySet;

namespace manifold {
  class Manifold;
  class Mesh;
};

namespace ManifoldUtils {

  const char* statusToString(manifold::Manifold::Error status);

  std::shared_ptr<ManifoldGeometry> createMutableManifoldFromPolySet(const PolySet& ps);
  std::shared_ptr<ManifoldGeometry> createMutableManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom);

  template <class TriangleMesh>
  std::shared_ptr<ManifoldGeometry> createMutableManifoldFromSurfaceMesh(const TriangleMesh& mesh);

  std::shared_ptr<const ManifoldGeometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op);
};