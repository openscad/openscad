// Workaround for OpenCSG with OSMesa on macOS:
// OpenCSG's internal GLAD loader hardcodes dlopening Apple's OpenGL.framework,
// which segfaults under OSMesa because CGLGetCurrentContext() returns NULL.
// This overrides OpenCSG::OpenGL::ensureFunctionPointers() to load GL functions
// via OSMesaGetProcAddress when an OSMesa context is active.
// TODO: Replace once OpenCSG upstream provides an official OpenGL proc-address injection API.
#include "openglExt.h"

#ifdef ENABLE_OSMESA
extern "C" void *OSMesaGetCurrentContext(void);
extern "C" void (*OSMesaGetProcAddress(const char *funcName))(void);
#endif

namespace OpenCSG {
namespace OpenGL {

void ensureFunctionPointers()
{
  static bool sHaveOpenGLFunctions = false;
  if (sHaveOpenGLFunctions) return;
#ifdef ENABLE_OSMESA
  if (OSMesaGetCurrentContext()) {
    gladLoadGL(reinterpret_cast<GLADloadfunc>(OSMesaGetProcAddress));
    sHaveOpenGLFunctions = true;
    return;
  }
#endif
  initExtensionLibrary();
  sHaveOpenGLFunctions = true;
}

}  // namespace OpenGL
}  // namespace OpenCSG
