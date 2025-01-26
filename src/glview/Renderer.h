#pragma once

#include "geometry/linalg.h"
#include "glview/ColorMap.h"
#include "core/enums.h"
#include "geometry/PolySet.h"
#include "core/Selection.h"
#include "glview/system-gl.h"

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

enum class ShaderType {
  NONE,
  EDGE_RENDERING,
  SELECT_RENDERING,
};

// Shader attribute identifiers
struct ShaderInfo {
  GLuint shader_program;
  ShaderType type;
  std::unordered_map<std::string, int> uniforms;
  std::unordered_map<std::string, int> attributes;
};

CSGMode getCsgMode(const bool highlight_mode, const bool background_mode, const OpenSCADOperator type = OpenSCADOperator::UNION);
std::string loadShaderSource(const std::string& name);
GLuint compileShaderProgram(const std::string& vs_str, const std::string& fs_str);

} // namespace RendererUtils

class Renderer
{
public:
  Renderer();
  virtual ~Renderer() = default;

  virtual void prepare(bool showedges, const RendererUtils::ShaderInfo *shaderinfo) = 0;
  virtual void draw(bool showedges, const RendererUtils::ShaderInfo *shaderinfo) const = 0;
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
  virtual void setColorScheme(const ColorScheme& cs);

  virtual std::vector<SelectedObject> findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance);

protected:
  std::map<ColorMode, Color4f> colormap_;
  const ColorScheme *colorscheme_{nullptr};
  void setupShader();
};
