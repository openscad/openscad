#pragma once

#include <utility>
#include <memory>
#include <vector>

#include "glview/VBORenderer.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "glview/ShaderUtils.h"
#include "glview/ColorMap.h"
#include "glview/VertexState.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALNefGeometry.h"
#endif

class CGALRenderer : public VBORenderer
{
public:
  CGALRenderer(const std::shared_ptr<const class Geometry>& geom);
  ~CGALRenderer() override;
  void prepare(const ShaderUtils::ShaderInfo *shaderinfo = nullptr) override;
  void draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo = nullptr) const override;
  void setColorScheme(const ColorScheme& cs) override;
  BoundingBox getBoundingBox() const override;

private:
  void addGeometry(const std::shared_ptr<const class Geometry>& geom);
#ifdef ENABLE_CGAL
  const std::vector<std::shared_ptr<class VBOPolyhedron>>& getPolyhedrons() const
  {
    return this->polyhedrons_;
  }
  void createPolyhedrons();
#endif

  // FIXME: PolySet and Polygon2d features are only needed for the lazy-union feature,
  // when a GeometryList may contain a mixture of CGAL and Polygon2d/PolySet geometries.
  void createPolySetStates();
  void createPolygonStates();
  void createPolygonSurfaceStates();
  void createPolygonEdgeStates();

  std::vector<std::shared_ptr<const class PolySet>> polysets_;
  std::vector<std::pair<std::shared_ptr<const Polygon2d>, std::shared_ptr<const PolySet>>> polygons_;
#ifdef ENABLE_CGAL
  std::vector<std::shared_ptr<class VBOPolyhedron>> polyhedrons_;
  std::vector<std::shared_ptr<const CGALNefGeometry>> nefPolyhedrons_;
#endif

  std::vector<VertexStateContainer> vertex_state_containers_;
};
