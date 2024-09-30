#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "glview/OffscreenContext.h"

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(
    uint32_t width, uint32_t height, uint32_t majorGLVersion, 
    uint32_t minorGLVersion, bool gles, bool compatibilityProfile);

}  // namespace offscreen_old
