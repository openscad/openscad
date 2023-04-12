#include "GLView.h"

GLView::GLView() {}
void GLView::setRenderer(Renderer *r) {}
void GLView::initializeGL() {}
void GLView::resizeGL(int w, int h) {}
void GLView::setCamera(const Camera& cam) {assert(false && "not implemented");}
void GLView::paintGL() {}
void GLView::showSmallaxes(const Color4f& col) {}
void GLView::showAxes(const Color4f& col) {}
void GLView::showCrosshairs(const Color4f& col) {}
void GLView::setColorScheme(const ColorScheme& cs){assert(false && "not implemented");}
void GLView::setColorScheme(const std::string& cs) {assert(false && "not implemented");}

#include "system-gl.h"

double gl_version() { return -1; }
std::string glew_dump() { return std::string("GL Renderer: NULLGL Glew\n"); }
std::string glew_extensions_dump() { return std::string("NULLGL Glew Extensions"); }
bool report_glerror(const char *function) { return false; }

#include "OpenGLContext.h"
std::vector<uint8_t> OpenGLContext::getFramebuffer() const { return {}; }

#include "fbo.h"

fbo_t *fbo_new() { return nullptr; }
void fbo_unbind(fbo_t *fbo) {}
void fbo_delete(fbo_t *fbo) {}
bool fbo_init(fbo_t *fbo, size_t width, size_t height) { return false; }
