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

