#pragma once

#include <utility>
#include <memory>
#include <vector>

#include "glview/VBORenderer.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
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
  std::shared_ptr<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) override;

private:
  void addGeometry(const std::shared_ptr<const class Geometry>& geom);
#ifdef ENABLE_CGAL
  const std::vector<std::shared_ptr<class VBOPolyhedron>>& getPolyhedrons() const { return this->polyhedrons_; }
  void createPolyhedrons();
#endif
  void createPolySetStates();
  void createPolygonStates();
  void createPolygonSurfaceStates();
  void createPolygonEdgeStates();
  bool last_render_state_; // FIXME: this is temporary to make switching between renderers seamless.

  std::vector<std::shared_ptr<const class PolySet>> polysets_;
  std::vector<std::pair<std::shared_ptr<const Polygon2d>, std::shared_ptr<const PolySet>>> polygons_;
#ifdef ENABLE_CGAL
  std::vector<std::shared_ptr<class VBOPolyhedron>> polyhedrons_;
  std::vector<std::shared_ptr<const CGAL_Nef_polyhedron>> nefPolyhedrons_;
#endif

  std::vector<VertexStateContainer> vertex_state_containers_;
};
