#ifndef SYSTEMGL_H_
#define SYSTEMGL_H_

#ifdef _WIN32
// Prevent obtuse compile errors on Win32/mingw32. This is related
// GLU Tessellation callback definitions, and how glew deals with them.
#include <windows.h>
#endif

#include <GL/glew.h>
#include <string>

std::string glew_dump(bool dumpall=false);
bool report_glerror(const char *task);

#endif
