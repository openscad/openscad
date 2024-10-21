#include "gui/MouseSelector.h"

#include "glview/system-gl.h"

#include <cstdint>
#include <QOpenGLFramebufferObject>
#include <string>
#include <memory>
/**
 * The selection is making use of a special shader, that renders each object in a color
 * that is derived from its index(), by using the first 24 bits of the identifier as a
 * 3-tuple for color.
 *
 * Roughly at most 1/3rd of the index()-es are rendered, therefore exhausting the keyspace
 * faster than expected.
 * If this ever becomes a problem, the index-mapping can be adjusted to use 10 up to 16 bit
 * per color channel to store the identifier.
 * Increasing this should be done carefully while testing on older graphics cards, they
 * might do "fancy" optimization.
 */

MouseSelector::MouseSelector(GLView *view) {
  this->view = view;
  if (view && !view->has_shaders) {
    return;
  }
  this->init_shader();

  if (view) this->reset(view);
}

/**
 * Resize the framebuffer whenever it changed
 */
void MouseSelector::reset(GLView *view) {
  this->view = view;
  this->setup_framebuffer(view);
}

/**
 * Initialize the used shaders and setup the shaderinfo_t struct
 */
void MouseSelector::init_shader() {
  /*
     Attributes:
   * frag_idcolor - (uniform) 24 bit of the selected object's id encoded into R/G/B components as float values
   */

  std::string vs_str = Renderer::loadShaderSource("MouseSelector.vert");
  std::string fs_str = Renderer::loadShaderSource("MouseSelector.frag");
  const char *vs_source = vs_str.c_str();
  const char *fs_source = fs_str.c_str();

  int shaderstatus;

  // Compile the shaders
  GL_CHECKD(auto vs = glCreateShader(GL_VERTEX_SHADER));
  glShaderSource(vs, 1, (const GLchar **)&vs_source, nullptr);
  glCompileShader(vs);
  glGetShaderiv(vs, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(vs, sizeof(logbuffer), &loglen, logbuffer);
    fprintf(stderr, __FILE__ ": OpenGL vertex shader Error:\n%.*s\n\n", loglen, logbuffer);
  }

  GL_CHECKD(auto fs = glCreateShader(GL_FRAGMENT_SHADER));
  glShaderSource(fs, 1, (const GLchar **)&fs_source, nullptr);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(fs, sizeof(logbuffer), &loglen, logbuffer);
    fprintf(stderr, __FILE__ ": OpenGL fragment shader Error:\n%.*s\n\n", loglen, logbuffer);
  }

  // Link
  auto selecthader_prog = glCreateProgram();
  glAttachShader(selecthader_prog, vs);
  glAttachShader(selecthader_prog, fs);
  GL_CHECKD(glLinkProgram(selecthader_prog));

  GLint status;
  glGetProgramiv(selecthader_prog, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(selecthader_prog, sizeof(logbuffer), &loglen, logbuffer);
    fprintf(stderr, __FILE__ ": OpenGL Program Linker Error:\n%.*s\n\n", loglen, logbuffer);
  } else {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(selecthader_prog, sizeof(logbuffer), &loglen, logbuffer);
    if (loglen > 0) {
      fprintf(stderr, __FILE__ ": OpenGL Program Link OK:\n%.*s\n\n", loglen, logbuffer);
    }
    glValidateProgram(selecthader_prog);
    glGetProgramInfoLog(selecthader_prog, sizeof(logbuffer), &loglen, logbuffer);
    if (loglen > 0) {
      fprintf(stderr, __FILE__ ": OpenGL Program Validation results:\n%.*s\n\n", loglen, logbuffer);
    }
  }

  this->shaderinfo.progid = selecthader_prog;
  this->shaderinfo.type = Renderer::SELECT_RENDERING;
  GLint identifier = glGetUniformLocation(selecthader_prog, "frag_idcolor");
  if (identifier < 0) {
    fprintf(stderr, __FILE__ ": OpenGL symbol retrieval went wrong, id is %i\n\n", identifier);
    this->shaderinfo.data.select_rendering.identifier = 0;
  } else {
    this->shaderinfo.data.select_rendering.identifier = identifier;
  }
}

/**
 * Resize or create the framebuffer
 */
void MouseSelector::setup_framebuffer(const GLView *view) {
  if (!this->framebuffer ||
      static_cast<unsigned int>(this->framebuffer->width()) != view->cam.pixel_width ||
      static_cast<unsigned int>(this->framebuffer->height()) != view->cam.pixel_height) {
    this->framebuffer = std::make_unique<QOpenGLFramebufferObject>(
      view->cam.pixel_width,
      view->cam.pixel_width,
      QOpenGLFramebufferObject::Depth);
    this->framebuffer->release();
  }
}

/**
 * Setup the shaders, Projection and Model matrix and call the given renderer.
 * The renderer has to make sure, that the colors are defined accordingly, or
 * the selection won't work.
 *
 * returns 0 if no object was found
 */
int MouseSelector::select(const Renderer *renderer, int x, int y) {
  // x/y is originated topleft, so turn y around
  y = this->view->cam.pixel_height - y;

  if (x > static_cast<int>(this->view->cam.pixel_width) || x < 0 ||
      y > static_cast<int>(this->view->cam.pixel_height) || y < 0) {
    return -1;
  }

  // Initialize GL to draw to texture
  // Ideally a texture of only 1x1 or 2x2 pixels as a subset of the viewing frustrum
  // of the currently selected frame.
  // For now, i will use a texture the same size as the normal viewport
  // and select the identifier at the mouse coordinates
  GL_CHECKD(this->framebuffer->bind());

  glClearColor(0, 0, 0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glViewport(0, 0, this->view->cam.pixel_width, this->view->cam.pixel_height);
  this->view->setupCamera();
  glTranslated(this->view->cam.object_trans.x(),
               this->view->cam.object_trans.y(),
               this->view->cam.object_trans.z());

  glDisable(GL_LIGHTING);
  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // call the renderer with the selector shader
  GL_CHECKD(renderer->draw(true, false, &this->shaderinfo));

  // Not strictly necessary, but a nop if not required.
  glFlush();
  glFinish();

  // Grab the color from the framebuffer and convert it back to an identifier
  GLubyte color[3] = { 0 };
  GL_CHECKD(glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, color));
  glDisable(GL_DEPTH_TEST);

  int index = (uint32_t)color[0] | ((uint32_t)color[1] << 8) | ((uint32_t)color[2] << 16);

  // Switch the active framebuffer back to the default
  this->framebuffer->release();

  return index;
}
