#include "GLView.h"

GLView::GLView() {}
void GLView::setRenderer(Renderer* r) {}
void GLView::initializeGL() {}
void GLView::resizeGL(int w, int h) {}
//void GLView::setupGimbalCamPerspective() {}
//void GLView::setupGimbalCamOrtho(double distance, bool offset) {}
//void GLView::setupVectorCamPerspective() {}
//void GLView::setupVectorCamOrtho(bool offset) {}
void GLView::setCamera(const Camera &cam ) {}
void GLView::paintGL() {}
//void GLView::vectorCamPaintGL() {}
//void GLView::gimbalCamPaintGL() {}
void GLView::showSmallaxes() {}
void GLView::showAxes() {}
void GLView::showCrosshairs() {}
void GLView::setColorScheme(const ColorScheme &cs){assert(false && "not implemented");}
void GLView::setColorScheme(const std::string &cs) {assert(false && "not implemented");}

#include "ThrownTogetherRenderer.h"

ThrownTogetherRenderer::ThrownTogetherRenderer(CSGChain *root_chain,
 CSGChain *highlights_chain, CSGChain *background_chain) {}
void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges) const {}
void ThrownTogetherRenderer::renderCSGChain(CSGChain *chain, bool
  highlight, bool background, bool showedges, bool fberror) const {}
BoundingBox ThrownTogetherRenderer::getBoundingBox() const {assert(false && "not implemented");}

#include "CGALRenderer.h"

CGALRenderer::CGALRenderer(shared_ptr<const class Geometry> geom) {}
CGALRenderer::~CGALRenderer() {}
void CGALRenderer::draw(bool showfaces, bool showedges) const {}
BoundingBox CGALRenderer::getBoundingBox() const {assert(false && "not implemented");}
void CGALRenderer::setColorScheme(const ColorScheme &cs){assert(false && "not implemented");}


#include "system-gl.h"

double gl_version() { return -1; }
std::string glew_dump() { return std::string("NULLGL Glew"); }
std::string glew_extensions_dump() { return std::string("NULLGL Glew Extensions"); }
bool report_glerror(const char * function) { return false; }

