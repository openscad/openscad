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

namespace RendererUtils {

#define CSGMODE_DIFFERENCE_FLAG 0x10

enum CSGMode {
  CSGMODE_NONE                  = 0x00,
  CSGMODE_NORMAL                = 0x01,
  CSGMODE_DIFFERENCE            = CSGMODE_NORMAL | CSGMODE_DIFFERENCE_FLAG,
  CSGMODE_BACKGROUND            = 0x02,
  CSGMODE_BACKGROUND_DIFFERENCE = CSGMODE_BACKGROUND | CSGMODE_DIFFERENCE_FLAG,
  CSGMODE_HIGHLIGHT             = 0x03,
  CSGMODE_HIGHLIGHT_DIFFERENCE  = CSGMODE_HIGHLIGHT | CSGMODE_DIFFERENCE_FLAG
};

CSGMode getCsgMode(const bool highlight_mode, const bool background_mode, const OpenSCADOperator type = OpenSCADOperator::UNION);
std::string loadShaderSource(const std::string& name);

} // namespace RendererUtils

class Renderer
{
public:
  enum class ShaderType {
    NONE,
    CSG_RENDERING,
    EDGE_RENDERING,
    SELECT_RENDERING,
  };

  /// Shader attribute identifiers
  struct ShaderInfo {
    int progid = 0;
    ShaderType type;
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
  [[nodiscard]] const Renderer::ShaderInfo& getShader() const { return renderer_shader_; }

  virtual void prepare(bool showfaces, bool showedges, const ShaderInfo *shaderinfo = nullptr) = 0;
  virtual void draw(bool showfaces, bool showedges, const ShaderInfo *shaderinfo = nullptr) const = 0;
  [[nodiscard]] virtual BoundingBox getBoundingBox() const = 0;


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

  bool getColor(ColorMode colormode, Color4f& col) const;
  virtual void setColor(const float color[4], const ShaderInfo *shaderinfo = nullptr) const;
  virtual void setColor(ColorMode colormode, const ShaderInfo *shaderinfo = nullptr) const;
  virtual Color4f setColor(ColorMode colormode, const float color[4], const ShaderInfo *shaderinfo = nullptr) const;
  virtual void setColorScheme(const ColorScheme& cs);

  virtual std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance);

protected:
  std::map<ColorMode, Color4f> colormap_;
  const ColorScheme *colorscheme_{nullptr};
  void setupShader();

private:
  ShaderInfo renderer_shader_;
};
