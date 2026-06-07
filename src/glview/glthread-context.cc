#include "glview/glthread-context.h"

// Thread-local storage for GL functions
thread_local QOpenGLFunctions *g_current_gl_functions = nullptr;

void setCurrentGLFunctions(QOpenGLFunctions *gl)
{
  g_current_gl_functions = gl;
}

QOpenGLFunctions *getCurrentGLFunctions()
{
  return g_current_gl_functions;
}
