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
  object_rot << 35, 0, 25;
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

  if (orthomode) setupOrtho();
  else setupPerspective();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(1.0f, 1.0f, 0.92f, 1.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  gluLookAt(this->camera_eye[0], this->camera_eye[1], this->camera_eye[2],
            this->camera_center[0], this->camera_center[1], this->camera_center[2], 
            0.0, 0.0, 1.0);

  // glRotated(object_rot[0], 1.0, 0.0, 0.0);
  // glRotated(object_rot[1], 0.0, 1.0, 0.0);
  // glRotated(object_rot[2], 0.0, 0.0, 1.0);

  // Large gray axis cross inline with the model
  // FIXME: This is always gray - adjust color to keep contrast with background
  if (showaxes)
  {
    glLineWidth(1);
    glColor3d(0.5, 0.5, 0.5);
    glBegin(GL_LINES);
    double l = 3*(this->camera_center - this->camera_eye).norm();
    glVertex3d(-l, 0, 0);
    glVertex3d(+l, 0, 0);
    glVertex3d(0, -l, 0);
    glVertex3d(0, +l, 0);
    glVertex3d(0, 0, -l);
    glVertex3d(0, 0, +l);
    glEnd();
  }

  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);

  glLineWidth(2);
  glColor3d(1.0, 0.0, 0.0);

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

/*
void OffscreenView::setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center)
{
  this->camera_eye = pos;
  this->camera_center = center;
}

*/
