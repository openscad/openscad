#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/Selection.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"
#include "glview/ColorMap.h"
#include "glview/ShaderUtils.h"
#include "glview/VertexState.h"
#include "glview/VBORenderer.h"

class PolySetRenderer : public VBORenderer
{
public:
  PolySetRenderer(const std::shared_ptr<const class Geometry>& geom);
  ~PolySetRenderer() override = default;
  void prepare(const ShaderUtils::ShaderInfo *shaderinfo) override;
  void draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo) const override;
  void setColorScheme(const ColorScheme& cs) override;
  BoundingBox getBoundingBox() const override;

  /**
   * @brief Search for a segment or vertex on the line between near_pt and far_pt (with some tolerance)
   * and closest to near_pt. Mostly that's true, but line segment searching seems a little whack. And if
   * there's any points "behind" near_pt but on the near_pt<->far_pt infinite line, which is chosen is
   * likely non-optimal.
   */
  std::vector<SelectedObject> findModelObject(const Vector3d& near_pt, const Vector3d& far_pt,
                                              int mouse_x, int mouse_y, double tolerance) override;

private:
  void addGeometry(const std::shared_ptr<const class Geometry>& geom);
  void createPolySetStates(const ShaderUtils::ShaderInfo *shaderinfo);
  void createPolygonStates();
  void createPolygonSurfaceStates();
  void createPolygonEdgeStates();

  void drawPolySets(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo) const;
  void drawPolygons() const;

  std::vector<std::shared_ptr<const class PolySet>> polysets_;
  std::vector<std::pair<std::shared_ptr<const Polygon2d>, std::shared_ptr<const PolySet>>> polygons_;

  std::vector<VertexStateContainer> polyset_vertex_state_containers_;
  std::vector<VertexStateContainer> polygon_vertex_state_containers_;
};
