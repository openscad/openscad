#ifndef SYSTEMGL_H_
#define SYSTEMGL_H_

#ifndef NULLGL

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

#else // NULLGL
#define GLint int
#define GLuint unsigned int
inline void glColor4fv( float *c ) {}
#endif // NULLGL

#include <string>

std::string glew_dump();
std::string glew_extensions_dump();
bool report_glerror(const char * function);

#endif // SYSTEMGL_H_
