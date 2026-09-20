#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "glview/OffscreenContext.h"

std::shared_ptr<OffscreenContext> CreateOffscreenContextWGL(
    size_t width, size_t height, size_t majorGLVersion, 
    size_t minorGLVersion, bool compatibilityProfile);

