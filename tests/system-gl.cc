
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

  cerr << " GLEW_ARB_framebuffer_object: " << ((GLEW_ARB_framebuffer_object==1) ? "yes" : "no" ) << endl
       << " GLEW_EXT_framebuffer_object: " << ((GLEW_EXT_framebuffer_object==1) ? "yes" : "no")  << endl
       << " GLEW_EXT_packed_depth_stencil: " << ((GLEW_EXT_packed_depth_stencil==1) ? "yes" : "no") << endl;
};

const char * gl_errors[] = {
  "​GL_INVALID_ENUM​", // 0x0500
  "GL_INVALID_VALUE", // 0x0501
  "GL_INVALID_OPERATION", // 0x0502
  "GL_OUT_OF_MEMORY" // 0x0503
};

bool report_glerror(const char * function)
{
  GLenum tGLErr = glGetError();
  if (tGLErr != GL_NO_ERROR) {
    if ( (tGLErr-0x500)<=3 && (tGLErr-0x500)>=0 )
      cerr << "OpenGL error " << gl_errors[tGLErr-0x500] << " after " << function << endl;
    else
      cerr << "OpenGL error 0x" << hex << tGLErr << " after " << function << endl;
    return true;
  }
  return false;
}

