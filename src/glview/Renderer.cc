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
  const GLuint shader_prog = RendererUtils::compileShaderProgram(
    RendererUtils::loadShaderSource("Preview.vert"),
    RendererUtils::loadShaderSource("Preview.frag"));

  renderer_shader_ = {
    .shader_program = shader_prog,
    .type = RendererUtils::ShaderType::EDGE_RENDERING,
    .uniforms = {
      {"color_area", glGetUniformLocation(shader_prog, "color_area")},
      {"color_edge", glGetUniformLocation(shader_prog, "color_edge")},
    },
    .attributes = {
      {"barycentric", glGetAttribLocation(shader_prog, "barycentric")},
    },
  };
}

bool Renderer::getColor(Renderer::ColorMode colormode, Color4f& col) const
{
  if (const auto it = colormap_.find(colormode); it != colormap_.end()) {
    col = it->second;
    return true;
  }
  return false;
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
void Renderer::setColorScheme(const ColorScheme& cs) {}
std::vector<SelectedObject> Renderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) { return {}; }

#endif //NULLGL
