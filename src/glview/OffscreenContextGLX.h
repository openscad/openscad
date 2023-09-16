#pragma once

#include <memory>

#include "OffscreenContext.h"

std::shared_ptr<OffscreenContext> CreateOffscreenContextGLX(
    size_t width, size_t height, size_t majorGLVersion,
    size_t minorGLVersion, bool gles, bool compatibilityProfile);
