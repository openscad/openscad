#pragma once

#include <memory>

#include "VBORenderer.h"
#ifdef ENABLE_CGAL
#include "CGAL_OGL_Polyhedron.h"
#include "CGAL_Nef_polyhedron.h"
#include "Selection.h"
#endif

class CGALRenderer : public VBORenderer
{
public:
  CGALRenderer(const std::shared_ptr<const class Geometry>& geom);
  ~CGALRenderer() override;
  void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) override;
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;
  void setColorScheme(const ColorScheme& cs) override;
  BoundingBox getBoundingBox() const override;
  std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) override;

private:
  void addGeometry(const std::shared_ptr<const class Geometry>& geom);
#ifdef ENABLE_CGAL
  const std::vector<std::shared_ptr<class CGAL_OGL_Polyhedron>>& getPolyhedrons() const { return this->polyhedrons; }
  void createPolyhedrons();
#endif
  void createPolySetStates();
  bool last_render_state; // FIXME: this is temporary to make switching between renderers seamless.

  std::vector<std::shared_ptr<const class PolySet>> polysets;
  std::vector<std::pair<std::shared_ptr<const Polygon2d>, std::shared_ptr<const PolySet>>> polygons;
#ifdef ENABLE_CGAL
  std::vector<std::shared_ptr<class CGAL_OGL_Polyhedron>> polyhedrons;
  std::vector<std::shared_ptr<const CGAL_Nef_polyhedron>> nefPolyhedrons;
#endif

  std::vector<std::shared_ptr<VertexState>> vertex_states;
  GLuint polyset_vertices_vbo{0};
  GLuint polyset_elements_vbo{0};
};
