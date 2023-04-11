#pragma once

#include <memory>

#include "OffscreenContext.h"

std::shared_ptr<OffscreenContext> CreateOffscreenContextNSOpenGL(
    uint32_t width, uint32_t height, uint32_t majorGLVersion, 
    uint32_t minorGLVersion);
