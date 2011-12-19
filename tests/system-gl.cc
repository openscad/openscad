
/* OpenGL helper functions */

#include <iostream>
#include <sstream>
#include <string>
#include "system-gl.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

string glew_dump(bool dumpall)
{
  stringstream out;
  out << "GLEW version: " << glewGetString(GLEW_VERSION) << endl
       << "GL Renderer: " << (const char *)glGetString(GL_RENDERER) << endl
       << "GL Vendor: " << (const char *)glGetString(GL_VENDOR) << endl
       << "OpenGL Version: " << (const char *)glGetString(GL_VERSION) << endl;

  out << "GL Extensions: " << endl;
  if (dumpall) {
    string extensions((const char *)glGetString(GL_EXTENSIONS));
    replace_all( extensions, " ", "\n " );
    out << " " << extensions << endl;
  }

  out << "GL_ARB_framebuffer_object: "
      << (glewIsSupported("GL_ARB_framebuffer_object") ? "yes" : "no")
      << endl
      << "GL_EXT_framebuffer_object: "
      << (glewIsSupported("GL_EXT_framebuffer_object") ? "yes" : "no")
      << endl
      << "GL_EXT_packed_depth_stencil: "
      << (glewIsSupported("GL_EXT_packed_depth_stencil") ? "yes" : "no")
      << endl;

  return out.str();
};

bool report_glerror(const char * function)
{
  GLenum tGLErr = glGetError();
  if (tGLErr != GL_NO_ERROR) {
    cerr << "OpenGL error 0x" << hex << tGLErr << ": " << gluErrorString(tGLErr) << " after " << function << endl;
    return true;
  }
  return false;
}

