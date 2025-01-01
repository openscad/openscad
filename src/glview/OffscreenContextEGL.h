#pragma once

#include <cstddef>
#include <memory>

#include "glview/OffscreenContext.h"

std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(
    size_t width, size_t height, size_t majorGLVersion, 
    size_t minorGLVersion, bool gles, bool compatibilityProfile);
