#pragma once

#include <cstdint>
#include <memory>

#include "glview/OffscreenContext.h"

std::shared_ptr<OffscreenContext> CreateOffscreenContextOSMesa(uint32_t width, uint32_t height,
                                                               uint32_t majorGLVersion,
                                                               uint32_t minorGLVersion,
                                                               bool compatibilityProfile);
