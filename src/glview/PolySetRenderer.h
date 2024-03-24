#pragma once

#include <memory>

#include "VBORenderer.h"
#include "Selection.h"

class PolySetRenderer : public VBORenderer
{
public:
  PolySetRenderer(const std::shared_ptr<const class Geometry>& geom);
  ~PolySetRenderer() override;
  void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) override;
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;
  void setColorScheme(const ColorScheme& cs) override;
  BoundingBox getBoundingBox() const override;
  std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) override;

private:
  void addGeometry(const std::shared_ptr<const class Geometry>& geom);
  void createVertexStates();

  std::vector<std::shared_ptr<const class PolySet>> polysets_;
  std::vector<std::shared_ptr<VertexState>> vertex_states_;
  GLuint vertices_vbo_{0};
  GLuint elements_vbo_{0};
};
