#include "OffscreenView.h"
#include "system-gl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <sstream>
#include "printutils.h"

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
  PRINT("OpenSCAD recommended OpenGL version is 2.0.");
}
#endif

bool OffscreenView::save(const std::string &filename)
{
  return save_framebuffer(this->ctx, filename.c_str());
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
