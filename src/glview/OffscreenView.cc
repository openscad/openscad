#include "OffscreenView.h"
#include "system-gl.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <sstream>
#include "printutils.h"

OffscreenView::OffscreenView(int width, int height)
{
  this->ctx = create_offscreen_context(width, height);
  if (this->ctx == nullptr) throw -1;

  this->fbo = fbo_new();
  if (!fbo_init(this->fbo, width, height)) throw -1;

  GLView::initializeGL();
  GLView::resizeGL(width, height);
}

OffscreenView::~OffscreenView()
{
  fbo_unbind(this->fbo);
  fbo_delete(this->fbo);
  teardown_offscreen_context(this->ctx);
}

#ifdef ENABLE_OPENCSG
void OffscreenView::display_opencsg_warning()
{
  LOG("OpenSCAD recommended OpenGL version is 2.0.");
}
#endif

bool OffscreenView::save(const char *filename) const
{
  return save_framebuffer(this->ctx, filename);
}

bool OffscreenView::save(std::ostream& output) const
{
  return save_framebuffer(this->ctx, output);
}

std::string OffscreenView::getRendererInfo() const
{
  return STR(glew_dump(), offscreen_context_getinfo(this->ctx));
}
