#pragma once

#include <GL/glew.h>

#ifdef __APPLE__
 #include <OpenGL/OpenGL.h>
#else
 #include <GL/gl.h>
 #include <GL/glu.h>
#endif

#include <string>

std::string glew_dump();
std::string glew_extensions_dump();
bool report_glerror(const char * function);
