#include "GLView.h"

GLView::GLView() {}
void GLView::setRenderer(Renderer* r) {}
void GLView::initializeGL() {}
void GLView::resizeGL(int w, int h) {}
void GLView::setCamera(const Camera &cam ) {assert(false && "not implemented");}
void GLView::paintGL() {}
void GLView::showSmallaxes(const Color4f &col) {}
void GLView::showAxes(const Color4f &col) {}
void GLView::showCrosshairs(const Color4f &col) {}
void GLView::setColorScheme(const ColorScheme &cs){assert(false && "not implemented");}
void GLView::setColorScheme(const std::string &cs) {assert(false && "not implemented");}

#include "ThrownTogetherRenderer.h"

ThrownTogetherRenderer::ThrownTogetherRenderer(shared_ptr<class CSGProducts> root_products,
                        shared_ptr<CSGProducts> highlight_products,
                        shared_ptr<CSGProducts> background_products) {}
void ThrownTogetherRenderer::draw(bool showfaces, bool showedges) const {};
BoundingBox ThrownTogetherRenderer::getBoundingBox() const {assert(false && "not implemented");}
void ThrownTogetherRenderer::renderCSGProducts(const CSGProducts &products, bool highlight_mode, bool background_mode, bool showedges, 
                        bool fberror) const {}
void ThrownTogetherRenderer::renderChainObject(const class CSGChainObject &csgobj, bool highlight_mode,
                        bool background_mode, bool showedges, bool fberror, OpenSCADOperator type) const {}

#include "CGALRenderer.h"

CGALRenderer::CGALRenderer(shared_ptr<const class Geometry> geom) {}
CGALRenderer::~CGALRenderer() {}
void CGALRenderer::draw(bool showfaces, bool showedges) const {}
BoundingBox CGALRenderer::getBoundingBox() const {assert(false && "not implemented");}
void CGALRenderer::setColorScheme(const ColorScheme &cs){assert(false && "not implemented");}



#include "system-gl.h"

double gl_version() { return -1; }
std::string glew_dump() { return std::string("GL Renderer: NULLGL Glew\n"); }
std::string glew_extensions_dump() { return std::string("NULLGL Glew Extensions"); }
bool report_glerror(const char * function) { return false; }

