#pragma once

#include "OpenGLContext.h"

class OffscreenContext : public OpenGLContext {
public:
  OffscreenContext(uint32_t width, uint32_t height) : OpenGLContext(width, height) {}
};
