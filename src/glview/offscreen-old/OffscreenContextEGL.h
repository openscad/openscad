#pragma once

#include <memory>
#include <string>

#include "OffscreenContext.h"

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(
    uint32_t width, uint32_t height, uint32_t majorGLVersion, 
    uint32_t minorGLVersion, bool gles, bool compatibilityProfile,
    const std::string& drmNode = "");

}  // namespace offscreen_old
