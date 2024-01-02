#pragma once

#include <memory>

#include "Renderer.h"
#ifdef ENABLE_CGAL
#include "CGAL_OGL_Polyhedron.h"
#include "CGAL_Nef_polyhedron.h"
#include "Selection.h"
#endif

class LegacyCGALRenderer : public Renderer
{
public:
  LegacyCGALRenderer(const std::shared_ptr<const class Geometry>& geom);
  ~LegacyCGALRenderer() override = default;
  void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) override;
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;
  void setColorScheme(const ColorScheme& cs) override;
  BoundingBox getBoundingBox() const override;
  std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) override;

private:
  void addGeometry(const std::shared_ptr<const class Geometry>& geom);
#ifdef ENABLE_CGAL
  const std::list<std::shared_ptr<class CGAL_OGL_Polyhedron>>& getPolyhedrons() const { return this->polyhedrons; }
  void createPolyhedrons();
#endif
  std::list<std::shared_ptr<const class PolySet>> polysets;
#ifdef ENABLE_CGAL
  std::list<std::shared_ptr<class CGAL_OGL_Polyhedron>> polyhedrons;
  std::list<std::shared_ptr<const CGAL_Nef_polyhedron>> nefPolyhedrons;
#endif
};
