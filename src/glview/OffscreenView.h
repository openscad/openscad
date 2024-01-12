#pragma once

#include <memory>
#include <string>
#include <ostream>

#include "GLView.h"
#include "OpenGLContext.h"
#include "fbo.h"

class OffscreenViewException : public std::runtime_error
{
public:
  OffscreenViewException(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

class OffscreenView : public GLView
{
public:
  OffscreenView(uint32_t width, uint32_t height);
  ~OffscreenView() override;
  bool save(std::ostream& output) const;
  std::shared_ptr<OpenGLContext> ctx;
  fbo_t *fbo;

  // overrides
  bool save(const char *filename) const override;
  [[nodiscard]] std::string getRendererInfo() const override;
#ifdef ENABLE_OPENCSG
  void display_opencsg_warning() override;
#endif
};
