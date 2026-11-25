#pragma once

#include <stdexcept>
#include <cstdint>
#include <memory>
#include <string>
#include <ostream>
#include <filesystem>

namespace fs = std::filesystem;

#include "glview/GLView.h"
#include "glview/OpenGLContext.h"
#include "glview/fbo.h"

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
  // TODO: Do we need to worry about deletion order?
  std::shared_ptr<OpenGLContext> ctx;
  std::unique_ptr<FBO> fbo;

  // overrides
  bool save(const fs::path& filename) const override;
  [[nodiscard]] std::string getRendererInfo() const override;
#ifdef ENABLE_OPENCSG
  void display_opencsg_warning() override;
#endif
};
