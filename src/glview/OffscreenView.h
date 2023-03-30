#pragma once

#include <memory>
#include <string>
#include <ostream>

#include "GLView.h"
#include "OpenGLContext.h"
#include "fbo.h"

class OffscreenView : public GLView
{
public:
  OffscreenView(unsigned int width, unsigned int height);
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
