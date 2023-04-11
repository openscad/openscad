#pragma once

#include <memory>
#include <string>

#include "OpenGLContext.h"

namespace OffscreenContextFactory {

struct ContextAttributes {
  unsigned int width;           // Context size in pixels
  unsigned int height;
  unsigned int majorGLVersion;  // Minimum OpenGL or GLES major version
  unsigned int minorGLVersion;  // Minimum OpenGL or GLES minor version
  bool gles;                    // Request a GLES context
  bool compatibilityProfile;    // Request a compatibility context (to support legacy OpenGL calls)
  std::string gpu;              // Request a specific GPU (for supported providers)
};

const char *defaultProvider();
std::shared_ptr<OpenGLContext> create(const std::string& provider, const ContextAttributes& attrib);

}  // namespace OffscreenContextFactory
