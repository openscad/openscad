#pragma once

#include "OpenGLContext.h"

class OffscreenContext : public OpenGLContext {
public:
  OffscreenContext(int width, int height) : OpenGLContext(width, height) {}
};
