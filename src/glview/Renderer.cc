#include "glview/Renderer.h"
#include "geometry/PolySet.h"
#include "geometry/Polygon2d.h"
#include "glview/ColorMap.h"
#include "utils/printutils.h"
#include "platform/PlatformUtils.h"
#include "glview/system-gl.h"

#include <sstream>
#include <Eigen/LU>
#include <fstream>
#include <string>
#include <vector>

#ifndef NULLGL


namespace {

GLuint compileShader(const std::string& name, GLuint shader_type) {
  auto shader_source = RendererUtils::loadShaderSource(name);
  const GLuint shader = glCreateShader(shader_type);
  auto *c_source = shader_source.c_str();
  glShaderSource(shader, 1, (const GLchar **)&c_source, nullptr);
  glCompileShader(shader);
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (!status) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(shader, sizeof(logbuffer), &loglen, logbuffer);
    PRINTDB("OpenGL shader compilation error:\n%s", logbuffer);
    return 0;
  }
  return shader;
}

}  // namespace

namespace RendererUtils {

CSGMode getCsgMode(const bool highlight_mode, const bool background_mode, const OpenSCADOperator type) {
  int csgmode = highlight_mode ? CSGMODE_HIGHLIGHT : (background_mode ? CSGMODE_BACKGROUND : CSGMODE_NORMAL);
  if (type == OpenSCADOperator::DIFFERENCE) csgmode |= CSGMODE_DIFFERENCE_FLAG;
  return static_cast<CSGMode>(csgmode);
}

std::string loadShaderSource(const std::string& name) {
  std::string shaderPath = (PlatformUtils::resourcePath("shaders") / name).string();
  std::ostringstream buffer;
  const std::ifstream f(shaderPath);
  if (f.is_open()) {
    buffer << f.rdbuf();
  } else {
    LOG(message_group::UI_Error, "Cannot open shader source file: '%1$s'", shaderPath);
  }
  return buffer.str();
}

GLuint compileShaderProgram(const std::string& vs_str, const std::string& fs_str) {
  int shaderstatus;
  const char *vs_source = vs_str.c_str();
  const char *fs_source = fs_str.c_str();
  // Compile the shaders
  GL_CHECKD(auto vs = glCreateShader(GL_VERTEX_SHADER));
  glShaderSource(vs, 1, (const GLchar **)&vs_source, nullptr);
  glCompileShader(vs);
  glGetShaderiv(vs, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(vs, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL vertex shader Error:\n%.*s\n\n", loglen, logbuffer);
    return 0;
  }

  GL_CHECKD(auto fs = glCreateShader(GL_FRAGMENT_SHADER));
  glShaderSource(fs, 1, (const GLchar **)&fs_source, nullptr);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(fs, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL fragment shader Error:\n%.*s\n\n", loglen, logbuffer);
    return 0;
  }

  // Link
  auto shader_prog = glCreateProgram();
  glAttachShader(shader_prog, vs);
  glAttachShader(shader_prog, fs);
  GL_CHECKD(glLinkProgram(shader_prog));

  GLint status;
  glGetProgramiv(shader_prog, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(shader_prog, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL Program Linker Error:\n%.*s\n\n", loglen, logbuffer);
    return 0;
  } else {
    glValidateProgram(shader_prog);
    glGetProgramiv(shader_prog, GL_VALIDATE_STATUS, &status);
    if (!status) {
      int loglen;
      char logbuffer[1000];
      glGetProgramInfoLog(shader_prog, sizeof(logbuffer), &loglen, logbuffer);
      // FIXME: Use OpenCAD log to error instead of stderr
      fprintf(stderr, __FILE__ ": OpenGL Program Validation results:\n%.*s\n\n", loglen, logbuffer);
      return 0;
    }
  }

  return shader_prog;
}

}  // namespace RendererUtils

Renderer::Renderer()
{
  PRINTD("Renderer() start");

  // Setup default colors
  // The main colors, MATERIAL and CUTOUT, come from this object's
  // colorscheme. Colorschemes don't currently hold information
  // for Highlight/Background colors
  // but it wouldn't be too hard to make them do so.

  // MATERIAL is set by this object's colorscheme
  // CUTOUT is set by this object's colorscheme
  colormap_[ColorMode::HIGHLIGHT] = {255, 81, 81, 128};
  colormap_[ColorMode::BACKGROUND] = {180, 180, 180, 128};
  // MATERIAL_EDGES is set by this object's colorscheme
  // CUTOUT_EDGES is set by this object's colorscheme
  colormap_[ColorMode::HIGHLIGHT_EDGES] = {255, 171, 86, 128};
  colormap_[ColorMode::BACKGROUND_EDGES] = {150, 150, 150, 128};

  Renderer::setColorScheme(ColorMap::inst()->defaultColorScheme());

  setupShader();
  PRINTD("Renderer() end");
}

void Renderer::setupShader() {
  renderer_shader_.progid = 0;

  const std::string vs_str = RendererUtils::loadShaderSource("Preview.vert");
  const std::string fs_str = RendererUtils::loadShaderSource("Preview.frag");
  const GLuint shader_prog = RendererUtils::compileShaderProgram(vs_str, fs_str);

  renderer_shader_.progid = shader_prog;
  renderer_shader_.type = RendererUtils::ShaderType::EDGE_RENDERING;
  renderer_shader_.data.color_rendering.color_area = glGetUniformLocation(shader_prog, "color1");
  renderer_shader_.data.color_rendering.color_edge = glGetUniformLocation(shader_prog, "color2");
  renderer_shader_.data.color_rendering.barycentric = glGetAttribLocation(shader_prog, "barycentric");
}

bool Renderer::getColor(Renderer::ColorMode colormode, Color4f& col) const
{
  if (const auto it = colormap_.find(colormode); it != colormap_.end()) {
    col = it->second;
    return true;
  }
  return false;
}

void Renderer::setColor(const float color[4], const RendererUtils::ShaderInfo *shaderinfo) const
{
  if (shaderinfo && shaderinfo->type != RendererUtils::ShaderType::EDGE_RENDERING) {
    return;
  }

  PRINTD("setColor a");
  Color4f col;
  getColor(ColorMode::MATERIAL, col);
  float c[4] = {color[0], color[1], color[2], color[3]};
  if (c[0] < 0) c[0] = col[0];
  if (c[1] < 0) c[1] = col[1];
  if (c[2] < 0) c[2] = col[2];
  if (c[3] < 0) c[3] = col[3];
  glColor4fv(c);
#ifdef ENABLE_OPENCSG
  if (shaderinfo) {
    glUniform4f(shaderinfo->data.color_rendering.color_area, c[0], c[1], c[2], c[3]);
    glUniform4f(shaderinfo->data.color_rendering.color_edge, (c[0] + 1) / 2, (c[1] + 1) / 2, (c[2] + 1) / 2, 1.0);
  }
#endif
}

// returns the color which has been set, which may differ from the color input parameter
Color4f Renderer::setColor(ColorMode colormode, const float color[4], const RendererUtils::ShaderInfo *shaderinfo) const
{
  PRINTD("setColor b");
  Color4f basecol;
  if (getColor(colormode, basecol)) {
    if (colormode == ColorMode::BACKGROUND || colormode != ColorMode::HIGHLIGHT) {
      basecol = {color[0] >= 0 ? color[0] : basecol[0],
                 color[1] >= 0 ? color[1] : basecol[1],
                 color[2] >= 0 ? color[2] : basecol[2],
                 color[3] >= 0 ? color[3] : basecol[3]};
    }
    setColor(basecol.data(), shaderinfo);
  }
  return basecol;
}

void Renderer::setColor(ColorMode colormode, const RendererUtils::ShaderInfo *shaderinfo) const
{
  PRINTD("setColor c");
  float c[4] = {-1, -1, -1, -1};
  setColor(colormode, c, shaderinfo);
}

/* fill colormap_ with matching entries from the colorscheme. note
   this does not change Highlight or Background colors as they are not
   represented in the colorscheme (yet). Also edgecolors are currently the
   same for CGAL & OpenCSG */
void Renderer::setColorScheme(const ColorScheme& cs) {
  PRINTD("setColorScheme");
  colormap_[ColorMode::MATERIAL] = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_FRONT_COLOR);
  colormap_[ColorMode::CUTOUT] = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_BACK_COLOR);
  colormap_[ColorMode::MATERIAL_EDGES] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_FRONT_COLOR);
  colormap_[ColorMode::CUTOUT_EDGES] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_BACK_COLOR);
  colormap_[ColorMode::EMPTY_SPACE] = ColorMap::getColor(cs, RenderColor::BACKGROUND_COLOR);
  colorscheme_ = &cs;
}


std::vector<SelectedObject> Renderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) { return {}; }
#else //NULLGL

Renderer::Renderer() : colorscheme_(nullptr) {}
bool Renderer::getColor(Renderer::ColorMode colormode, Color4f& col) const { return false; }
std::string RendererUtils::loadShaderSource(const std::string& name) { return ""; }
void Renderer::setColor(const float color[4], const RendererUtils::ShaderInfo *shaderinfo) const {}
Color4f Renderer::setColor(ColorMode colormode, const float color[4], const RendererUtils::ShaderInfo *shaderinfo) const { return {}; }
void Renderer::setColor(ColorMode colormode, const RendererUtils::ShaderInfo *shaderinfo) const {}
void Renderer::setColorScheme(const ColorScheme& cs) {}
std::vector<SelectedObject> Renderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) { return {}; }

#endif //NULLGL
