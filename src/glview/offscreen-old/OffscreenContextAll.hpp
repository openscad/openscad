// Functions shared by OffscreenContext[platform].cc
// #include this directly after definition of struct OffscreenContext.

#include <vector>
#include <ostream>
#include "imageutils.h"
#include "glew-utils.h"
#include "printutils.h"

//	Called by create_offscreen_context() from platform-specific code.
std::shared_ptr<OffscreenContext> create_offscreen_context_common(std::shared_ptr<OffscreenContext> ctx)
{
  if (!ctx) return nullptr;

  if (!initializeGlew()) return nullptr;
#ifdef USE_GLAD
  // FIXME: We could ask for gladLoaderLoadGLES2() here instead
  const auto version = gladLoaderLoadGL();
  if (version == 0) {
    // FIXME: Can we figure out why?
    std::cerr << "Unable to init GLAD" << std::endl;
    return nullptr;
  }
  // FIXME: Only if verbose
  LOG("GLAD: Loaded OpenGL %1$d.%2$d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
#endif

  return ctx;
}

