#pragma once

#include "glview/GLView.h"
#include "glview/Renderer.h"
#include <QOpenGLFramebufferObject>

#include <memory>

/**
 * Grab the of the Tree element that was rendered at a specific location
 */
class MouseSelector
{
public:
  MouseSelector(GLView *view);

  /// Resize the renderbuffer
  void reset(GLView *view);

  int select(const Renderer *renderer, int x, int y);

  Renderer::shaderinfo_t shaderinfo;

private:
  void init_shader();
  void setup_framebuffer(const GLView *view);

  std::unique_ptr<QOpenGLFramebufferObject> framebuffer;

  GLView *view;
};
