#include <GL/glew.h>
#include "OffscreenView.h"
#include "system-gl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <sstream>

OffscreenView::OffscreenView(size_t width, size_t height)
{
  this->ctx = create_offscreen_context(width, height);
  if ( this->ctx == NULL ) throw -1;
  GLView::initializeGL();
  GLView::resizeGL(width, height);
}

OffscreenView::~OffscreenView()
{
  teardown_offscreen_context(this->ctx);
}

#ifdef ENABLE_OPENCSG
void OffscreenView::display_opencsg_warning()
{
    fprintf(stderr, "OpenSCAD recommended OpenGL version is 2.0. \n");
}
#endif

void OffscreenView::paintGL()
{
  glEnable(GL_LIGHTING);

  if (orthomode) setupVectorCamOrtho();
  else setupVectorCamPerspective();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(1.0f, 1.0f, 0.92f, 1.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  gluLookAt(vcam.eye[0], vcam.eye[1], vcam.eye[2],
            vcam.center[0], vcam.center[1], vcam.center[2],
            0.0, 0.0, 1.0);

	// fixme - showcrosshairs doesnt work with vector camera
  // if (showcrosshairs) GLView::showCrosshairs();

  if (showaxes) GLView::showAxes();

  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);

  glLineWidth(2);
  glColor3d(1.0, 0.0, 0.0);

	//FIXME showSmallAxes wont work with vector camera
  //if (showaxes) GLView::showSmallaxes();

  if (this->renderer) {
    this->renderer->draw(showfaces, showedges);
  }
}

bool OffscreenView::save(const char *filename)
{
  return save_framebuffer(this->ctx, filename);
}

bool OffscreenView::save(std::ostream &output)
{
  return save_framebuffer(this->ctx, output);
}

std::string OffscreenView::getRendererInfo() const
{
  std::stringstream out;

  out << glew_dump()
      << offscreen_context_getinfo(this->ctx);

  return out.str();
}
