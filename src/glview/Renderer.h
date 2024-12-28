#pragma once

#include "geometry/linalg.h"
#include "glview/ColorMap.h"
#include "core/enums.h"
#include "geometry/PolySet.h"
#include "core/Selection.h"

#ifdef _MSC_VER // NULL
#include <map>
#include <cstdlib>
#endif

#include <string>
#include <vector>

class Renderer
{
public:
  enum shader_type_t {
    NONE,
    CSG_RENDERING,
    EDGE_RENDERING,
    SELECT_RENDERING,
  };

  /// Shader attribute identifiers
  struct shaderinfo_t {
    int progid = 0;
    shader_type_t type;
    union {
      struct {
        int color_area;
        int color_edge;
        // barycentric coordinates of the current vertex
        int barycentric;
      } csg_rendering;
      struct {
        int identifier;
      } select_rendering;
    } data;
  };


  Renderer();
  virtual ~Renderer() = default;
  virtual void resize(int w, int h);
  [[nodiscard]] virtual inline const Renderer::shaderinfo_t& getShader() const { return renderer_shader; }

  static std::string loadShaderSource(const std::string& name);
  virtual void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) {}
  virtual void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const = 0;
  [[nodiscard]] virtual BoundingBox getBoundingBox() const = 0;

#define CSGMODE_DIFFERENCE_FLAG 0x10
  enum csgmode_e {
    CSGMODE_NONE                  = 0x00,
    CSGMODE_NORMAL                = 0x01,
    CSGMODE_DIFFERENCE            = CSGMODE_NORMAL | CSGMODE_DIFFERENCE_FLAG,
    CSGMODE_BACKGROUND            = 0x02,
    CSGMODE_BACKGROUND_DIFFERENCE = CSGMODE_BACKGROUND | CSGMODE_DIFFERENCE_FLAG,
    CSGMODE_HIGHLIGHT             = 0x03,
    CSGMODE_HIGHLIGHT_DIFFERENCE  = CSGMODE_HIGHLIGHT | CSGMODE_DIFFERENCE_FLAG
  };

  enum class ColorMode {
    NONE,
    MATERIAL,
    CUTOUT,
    HIGHLIGHT,
    BACKGROUND,
    MATERIAL_EDGES,
    CUTOUT_EDGES,
    HIGHLIGHT_EDGES,
    BACKGROUND_EDGES,
    CGAL_FACE_2D_COLOR,
    CGAL_EDGE_2D_COLOR,
    EMPTY_SPACE
  };

  virtual bool getColor(ColorMode colormode, Color4f& col) const;
  virtual void setColor(const float color[4], const shaderinfo_t *shaderinfo = nullptr) const;
  virtual void setColor(ColorMode colormode, const shaderinfo_t *shaderinfo = nullptr) const;
  virtual Color4f setColor(ColorMode colormode, const float color[4], const shaderinfo_t *shaderinfo = nullptr) const;
  virtual void setColorScheme(const ColorScheme& cs);

  virtual std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance);

  [[nodiscard]] static csgmode_e get_csgmode(const bool highlight_mode, const bool background_mode, const OpenSCADOperator type = OpenSCADOperator::UNION);
protected:
  std::map<ColorMode, Color4f> colormap;
  const ColorScheme *colorscheme{nullptr};
  void setupShader();

private:
  shaderinfo_t renderer_shader;
};
