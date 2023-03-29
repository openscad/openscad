#pragma once

#include <memory>
#include <string>

#include "OffscreenContext.h"

std::shared_ptr<OffscreenContext> CreateOffscreenContextWGL(
    unsigned int width, unsigned int height, unsigned int majorGLVersion, 
    unsigned int minorGLVersion, bool compatibilityProfile);