#include "glview/GLView.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <cstddef>
#include <string>
#include <vector>

GLView::GLView() {}
GLView::~GLView() {}
void GLView::setRenderer(std::shared_ptr<Renderer>) {}
void GLView::initializeGL() {}
void GLView::resizeGL(int w, int h) {}
void GLView::setCamera(const Camera& cam) {assert(false && "not implemented");}
void GLView::paintGL() {}
void GLView::showSmallaxes(const Color4f& col) {}
void GLView::showAxes(const Color4f& col) {}
void GLView::showCrosshairs(const Color4f& col) {}
void GLView::setColorScheme(const ColorScheme& cs){assert(false && "not implemented");}
void GLView::setColorScheme(const std::string& cs) {assert(false && "not implemented");}

#include "glview/system-gl.h"

double gl_version() { return -1; }
std::string gl_dump() { return {"GL Renderer: NULLGL\n"}; }
std::string gl_extensions_dump() { return {"NULLGL Extensions"}; }
bool report_glerror(const char *function) { return false; }

#include "glview/OpenGLContext.h"
std::vector<uint8_t> OpenGLContext::getFramebuffer() const { return {}; }

#include "glview/fbo.h"

FBO::FBO(int, int, bool ) {}
bool FBO::resize(size_t, size_t) { return false; }
GLuint FBO::bind() { return 0; }
void FBO::unbind() {}
void FBO::destroy() {}
std::unique_ptr<FBO> createFBO(int, int) {return nullptr;}