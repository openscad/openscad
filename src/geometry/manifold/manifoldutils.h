#pragma once

#include <memory>
#include "geometry/Geometry.h"
#include "core/enums.h"
#include "geometry/manifold/ManifoldGeometry.h"

namespace ManifoldUtils {

  const char* statusToString(manifold::Manifold::Error status);

  std::shared_ptr<ManifoldGeometry> createManifoldFromPolySet(const PolySet& ps);
  std::shared_ptr<const ManifoldGeometry> createManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom);

  template <class TriangleMesh>
  std::shared_ptr<ManifoldGeometry> createManifoldFromSurfaceMesh(const TriangleMesh& mesh);

  std::shared_ptr<ManifoldGeometry> applyOperator3DManifold(const Geometry::Geometries& children, OpenSCADOperator op);

  Polygon2d polygonsToPolygon2d(const manifold::Polygons& polygons);

#ifdef ENABLE_CGAL
  // FIXME: This shouldn't return const, but it does due to internal implementation details.
  std::shared_ptr<const Geometry> applyMinkowskiManifold(const Geometry::Geometries& children);
#endif

  std::unique_ptr<PolySet> createTriangulatedPolySetFromPolygon2d(const Polygon2d& polygon2d);
};
