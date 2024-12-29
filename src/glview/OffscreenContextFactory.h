#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "glview/OpenGLContext.h"

namespace OffscreenContextFactory {

struct ContextAttributes {
  uint32_t width;            // Context size in pixels
  uint32_t height;           // 
  uint32_t majorGLVersion;   // Minimum OpenGL or GLES major version
  uint32_t minorGLVersion;   // Minimum OpenGL or GLES minor version
  bool gles;                 // Request a GLES context
  bool compatibilityProfile; // Request a compatibility context (to support legacy OpenGL calls)
};

const char *defaultProvider();
std::shared_ptr<OpenGLContext> create(const std::string& provider, const ContextAttributes& attrib);

}  // namespace OffscreenContextFactory
