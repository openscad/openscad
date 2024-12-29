#pragma once

#include <utility>
#include <memory>
#include <vector>

#include "glview/Renderer.h"
#include "geometry/PolySet.h"
#include "geometry/Polygon2d.h"
#ifdef ENABLE_CGAL
#include "glview/cgal/CGAL_OGL_Polyhedron.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include "core/Selection.h"
#endif

class LegacyCGALRenderer : public Renderer
{
public:
  LegacyCGALRenderer(const std::shared_ptr<const Geometry>& geom);
  ~LegacyCGALRenderer() override = default;
  void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) override;
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;
  void setColorScheme(const ColorScheme& cs) override;
  BoundingBox getBoundingBox() const override;
  std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) override;

private:
  void addGeometry(const std::shared_ptr<const Geometry>& geom);
#ifdef ENABLE_CGAL
  const std::vector<std::shared_ptr<CGAL_OGL_Polyhedron>>& getPolyhedrons() const { return this->polyhedrons; }
  void createPolyhedrons();
#endif
  std::vector<std::shared_ptr<const PolySet>> polysets;
  // We're using a pair: The PolySet can represent the polygon itself, 
  // and we use the Polygon2d to render outlines
  std::vector<std::pair<std::shared_ptr<const Polygon2d>, std::shared_ptr<const PolySet>>> polygons;
#ifdef ENABLE_CGAL
  std::vector<std::shared_ptr<CGAL_OGL_Polyhedron>> polyhedrons;
  std::vector<std::shared_ptr<const CGAL_Nef_polyhedron>> nefPolyhedrons;
#endif
};
