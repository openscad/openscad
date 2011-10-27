
/* OpenGL helper functions */

#include <iostream>
#include "system-gl.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

void glew_dump() {
  cerr << "GLEW version: " << glewGetString(GLEW_VERSION) << endl
       << "Renderer: " << (const char *)glGetString(GL_RENDERER) << endl
       << "Vendor: " << (const char *)glGetString(GL_VENDOR) << endl
       << "OpenGL version: " << (const char *)glGetString(GL_VERSION) << endl;

  bool dumpall = false;
  if (dumpall) {
    string extensions((const char *)glGetString(GL_EXTENSIONS));
    replace_all( extensions, " ", "\n " );
    cerr << "Extensions: " << endl << " " << extensions << endl;
  }

  cerr << " GL_ARB_framebuffer_object: " 
       << (glewIsSupported("GL_ARB_framebuffer_object") ? "yes" : "no")
       << endl
       << " GL_EXT_framebuffer_object: " 
       << (glewIsSupported("GL_EXT_framebuffer_object") ? "yes" : "no")  
       << endl
       << " GL_EXT_packed_depth_stencil: " 
       << (glewIsSupported("GL_EXT_packed_depth_stencil") ? "yes" : "no") 
       << endl;
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

