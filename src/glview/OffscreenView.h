#pragma once

#include <stdexcept>
#include <cstdint>
#include <memory>
#include <string>
#include <ostream>

#include "glview/GLView.h"
#include "glview/OpenGLContext.h"
#include "glview/fbo.h"
#include "io/depthmap.h"

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
  //! Write the depth buffer as a PNG in the given profile, rather than the
  //! color buffer. Must be called after paintGL(), like save().
  bool saveDepth(std::ostream& output, DepthProfile profile) const;
  bool saveDepth(std::ostream& output, const DepthmapOptions& options) const;
  // TODO: Do we need to worry about deletion order?
  std::shared_ptr<OpenGLContext> ctx;
  std::unique_ptr<FBO> fbo;

  // overrides
  bool save(const char *filename) const override;
  [[nodiscard]] std::string getRendererInfo() const override;
#ifdef ENABLE_OPENCSG
  void display_opencsg_warning() override;
#endif
};
