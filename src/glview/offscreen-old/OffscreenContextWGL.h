#pragma once

#include <cstdint>
#include <memory>

#include "glview/OffscreenContext.h"

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextWGL(
    uint32_t width, uint32_t height, uint32_t majorGLVersion, 
    uint32_t minorGLVersion, bool compatibilityProfile);

}  // namespace offscreen_old
