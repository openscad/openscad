#pragma once

#include <memory>
#include <string>

#include "OpenGLContext.h"

namespace OffscreenContextFactory {

struct ContextAttributes {
  unsigned int width;
  unsigned int height;
  unsigned int majorGLVersion;
  unsigned int minorGLVersion;
  bool gles;
  bool compatibilityProfile;
  std::string gpu;
  bool invisible;
};

const char *defaultProvider();
std::shared_ptr<OpenGLContext> create(const std::string& provider, const ContextAttributes& attrib);

}  // namespace OffscreenContextFactory
