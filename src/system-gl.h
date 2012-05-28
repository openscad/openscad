#ifndef SYSTEMGL_H_
#define SYSTEMGL_H_

#include <GL/glew.h>

#ifdef __APPLE__
 #include <OpenGL/OpenGL.h>
#else
 #include <GL/gl.h>
 #include <GL/glu.h>
 #ifdef _WIN32
  #include <windows.h> // For the CALLBACK macro
 #endif
#endif

#endif
