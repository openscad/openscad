#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "geometry/linalg.h"
#include "glview/Camera.h"
#include "glview/ColorMap.h"
#include "glview/Renderer.h"
#include "glview/fbo.h"
#include "glview/GLView.h"
#include "glview/OpenGLContext.h"
#include "glview/system-gl.h"

bool FBO::resize(size_t, size_t) { return false; }
FBO::FBO(int, int, bool) {}
GLuint FBO::bind() { return 0; }
GLView::~GLView() = default;
GLView::GLView() = default;
std::string gl_dump() { return {"GL Renderer: NULLGL\n"}; }
std::string gl_extensions_dump() { return {"NULLGL Extensions"}; }
std::unique_ptr<FBO> createFBO(int, int) { return nullptr; }
std::vector<uint8_t> OpenGLContext::getFramebuffer() const { return {}; }
void FBO::destroy() {}
void FBO::unbind() {}
void GLView::initializeGL() {}
void GLView::paintGL() {}
void GLView::resizeGL(int w, int h) {}
void GLView::setCamera(const Camera& /*cam*/) { assert(false && "not implemented"); }
void GLView::setColorScheme(const ColorScheme& /*cs*/) { assert(false && "not implemented"); }
void GLView::setColorScheme(const std::string& /*cs*/) { assert(false && "not implemented"); }
void GLView::setRenderer(std::shared_ptr<Renderer>) {}
void GLView::showAxes(const Color4f& col) {}
void GLView::showCrosshairs(const Color4f& col) {}
void GLView::showSmallaxes(const Color4f& col) {}