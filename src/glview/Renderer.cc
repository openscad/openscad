#include "Renderer.h"
#include "PolySet.h"
#include "Polygon2d.h"
#include "ColorMap.h"
#include "printutils.h"
#include "PlatformUtils.h"
#include "system-gl.h"

#include <Eigen/LU>
#include <fstream>

#ifndef NULLGL

Renderer::Renderer()
{
  PRINTD("Renderer() start");

  renderer_shader.progid = 0;

  // Setup default colors
  // The main colors, MATERIAL and CUTOUT, come from this object's
  // colorscheme. Colorschemes don't currently hold information
  // for Highlight/Background colors
  // but it wouldn't be too hard to make them do so.

  // MATERIAL is set by this object's colorscheme
  // CUTOUT is set by this object's colorscheme
  colormap[ColorMode::HIGHLIGHT] = {255, 81, 81, 128};
  colormap[ColorMode::BACKGROUND] = {180, 180, 180, 128};
  // MATERIAL_EDGES is set by this object's colorscheme
  // CUTOUT_EDGES is set by this object's colorscheme
  colormap[ColorMode::HIGHLIGHT_EDGES] = {255, 171, 86, 128};
  colormap[ColorMode::BACKGROUND_EDGES] = {150, 150, 150, 128};

  Renderer::setColorScheme(ColorMap::inst()->defaultColorScheme());

  std::string vs_str = Renderer::loadShaderSource("Preview.vert");
  std::string fs_str = Renderer::loadShaderSource("Preview.frag");
  const char *vs_source = vs_str.c_str();
  const char *fs_source = fs_str.c_str();

  GLint status;
  GLenum err;
  auto vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, (const GLchar **)&vs_source, nullptr);
  glCompileShader(vs);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
    return;
  }
  glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(vs, sizeof(logbuffer), &loglen, logbuffer);
    PRINTDB("OpenGL Program Compile Vertex Shader Error:\n%s", logbuffer);
    return;
  }

  auto fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, (const GLchar **)&fs_source, nullptr);
  glCompileShader(fs);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
    return;
  }
  glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(fs, sizeof(logbuffer), &loglen, logbuffer);
    PRINTDB("OpenGL Program Compile Fragment Shader Error:\n%s", logbuffer);
    return;
  }

  auto edgeshader_prog = glCreateProgram();
  glAttachShader(edgeshader_prog, vs);
  glAttachShader(edgeshader_prog, fs);
  glLinkProgram(edgeshader_prog);

  err = glGetError();
  if (err != GL_NO_ERROR) {
    PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
    return;
  }

  glGetProgramiv(edgeshader_prog, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
    PRINTDB("OpenGL Program Linker Error:\n%s", logbuffer);
    return;
  }

  int loglen;
  char logbuffer[1000];
  glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
  if (loglen > 0) {
    PRINTDB("OpenGL Program Link OK:\n%s", logbuffer);
  }
  glValidateProgram(edgeshader_prog);
  glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
  if (loglen > 0) {
    PRINTDB("OpenGL Program Validation results:\n%s", logbuffer);
  }

  renderer_shader.progid = edgeshader_prog;
  renderer_shader.type = EDGE_RENDERING;
  renderer_shader.data.csg_rendering.color_area = glGetUniformLocation(edgeshader_prog, "color1"); // 1
  renderer_shader.data.csg_rendering.color_edge = glGetUniformLocation(edgeshader_prog, "color2"); // 2
  renderer_shader.data.csg_rendering.barycentric = glGetAttribLocation(edgeshader_prog, "barycentric"); // 3

  PRINTD("Renderer() end");
}

void Renderer::resize(int /*w*/, int /*h*/)
{
}

bool Renderer::getColor(Renderer::ColorMode colormode, Color4f& col) const
{
  if (colormode == ColorMode::NONE) return false;
  if (colormap.count(colormode) > 0) {
    col = colormap.at(colormode);
    return true;
  }
  return false;
}

std::string Renderer::loadShaderSource(const std::string& name) {
  std::string shaderPath = (PlatformUtils::resourcePath("shaders") / name).string();
  std::ostringstream buffer;
  std::ifstream f(shaderPath);
  if (f.is_open()) {
    buffer << f.rdbuf();
  } else {
    LOG(message_group::UI_Error, "Cannot open shader source file: '%1$s'", shaderPath);
  }
  return buffer.str();
}

Renderer::csgmode_e Renderer::get_csgmode(const bool highlight_mode, const bool background_mode, const OpenSCADOperator type) {
  int csgmode = highlight_mode ? CSGMODE_HIGHLIGHT : (background_mode ? CSGMODE_BACKGROUND : CSGMODE_NORMAL);
  if (type == OpenSCADOperator::DIFFERENCE) csgmode |= CSGMODE_DIFFERENCE_FLAG;
  return csgmode_e(csgmode);
}

void Renderer::setColor(const float color[4], const shaderinfo_t *shaderinfo) const
{
  if (shaderinfo && shaderinfo->type != EDGE_RENDERING) {
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
    glUniform4f(shaderinfo->data.csg_rendering.color_area, c[0], c[1], c[2], c[3]);
    glUniform4f(shaderinfo->data.csg_rendering.color_edge, (c[0] + 1) / 2, (c[1] + 1) / 2, (c[2] + 1) / 2, 1.0);
  }
#endif
}

// returns the color which has been set, which may differ from the color input parameter
Color4f Renderer::setColor(ColorMode colormode, const float color[4], const shaderinfo_t *shaderinfo) const
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

void Renderer::setColor(ColorMode colormode, const shaderinfo_t *shaderinfo) const
{
  PRINTD("setColor c");
  float c[4] = {-1, -1, -1, -1};
  setColor(colormode, c, shaderinfo);
}

/* fill this->colormap with matching entries from the colorscheme. note
   this does not change Highlight or Background colors as they are not
   represented in the colorscheme (yet). Also edgecolors are currently the
   same for CGAL & OpenCSG */
void Renderer::setColorScheme(const ColorScheme& cs) {
  PRINTD("setColorScheme");
  colormap[ColorMode::MATERIAL] = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_FRONT_COLOR);
  colormap[ColorMode::CUTOUT] = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_BACK_COLOR);
  colormap[ColorMode::MATERIAL_EDGES] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_FRONT_COLOR);
  colormap[ColorMode::CUTOUT_EDGES] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_BACK_COLOR);
  colormap[ColorMode::EMPTY_SPACE] = ColorMap::getColor(cs, RenderColor::BACKGROUND_COLOR);
  this->colorscheme = &cs;
}


std::vector<SelectedObject> Renderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) { return std::vector<SelectedObject>(); }
#else //NULLGL

Renderer::Renderer() : colorscheme(nullptr) {}
void Renderer::resize(int /*w*/, int /*h*/) {}
bool Renderer::getColor(Renderer::ColorMode colormode, Color4f& col) const { return false; }
std::string Renderer::loadShaderSource(const std::string& name) { return ""; }
void Renderer::setColor(const float color[4], const shaderinfo_t *shaderinfo) const {}
Color4f Renderer::setColor(ColorMode colormode, const float color[4], const shaderinfo_t *shaderinfo) const { return {}; }
void Renderer::setColor(ColorMode colormode, const shaderinfo_t *shaderinfo) const {}
void Renderer::setColorScheme(const ColorScheme& cs) {}
std::vector<SelectedObject> Renderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) { return std::vector<SelectedObject>(); }

#endif //NULLGL
