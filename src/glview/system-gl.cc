
/* OpenGL helper functions */

#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include "system-gl.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "printutils.h"

double gl_version()
{
  std::string tmp((const char *)glGetString(GL_VERSION));
  std::vector<std::string> strs;
  boost::split(strs, tmp, boost::is_any_of("."));
  std::stringstream out;
  if (strs.size() >= 2) {
    out << strs[0] << "." << strs[1];
  } else {
    out << "0.0";
  }
  double d;
  out >> d;
  return d;
}

std::string glew_extensions_dump()
{
  std::string tmp;
  if (gl_version() >= 3.0) {
    GLint numexts = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numexts);
    for (int i = 0; i < numexts; ++i) {
      tmp += (const char *) glGetStringi(GL_EXTENSIONS, i);
      tmp += " ";
    }
  } else {
    tmp = (const char *) glGetString(GL_EXTENSIONS);
  }
  std::vector<std::string> extensions;
  boost::split(extensions, tmp, boost::is_any_of(" "));
  std::sort(extensions.begin(), extensions.end());
  std::ostringstream out;
  out << "GL Extensions:";
  for (auto& extension : extensions) {
    out << extension << "\n";
  }
  return out.str();
}

std::string glew_dump()
{
  GLint rbits, gbits, bbits, abits, dbits, sbits;
  glGetIntegerv(GL_RED_BITS, &rbits);
  glGetIntegerv(GL_GREEN_BITS, &gbits);
  glGetIntegerv(GL_BLUE_BITS, &bbits);
  glGetIntegerv(GL_ALPHA_BITS, &abits);
  glGetIntegerv(GL_DEPTH_BITS, &dbits);
  glGetIntegerv(GL_STENCIL_BITS, &sbits);

  std::ostringstream out;
  out << "GLEW version: " << glewGetString(GLEW_VERSION)
      << "\nOpenGL Version: " << (const char *)glGetString(GL_VERSION)
      << "\nGL Renderer: " << (const char *)glGetString(GL_RENDERER)
      << "\nGL Vendor: " << (const char *)glGetString(GL_VENDOR)
      << boost::format("\nRGBA(%d%d%d%d), depth(%d), stencil(%d)") %
    rbits % gbits % bbits % abits % dbits % sbits;
  out << "\nGL_ARB_framebuffer_object: "
      << (glewIsSupported("GL_ARB_framebuffer_object") ? "yes" : "no")
      << "\nGL_EXT_framebuffer_object: "
      << (glewIsSupported("GL_EXT_framebuffer_object") ? "yes" : "no")
      << "\nGL_EXT_packed_depth_stencil: "
      << (glewIsSupported("GL_EXT_packed_depth_stencil") ? "yes" : "no")
      << "\n";
  return out.str();
}
