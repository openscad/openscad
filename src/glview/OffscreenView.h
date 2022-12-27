#pragma once

#include "OffscreenContext.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <iostream>
#include "GLView.h"

class OffscreenView : public GLView
{
public:
  OffscreenView(int width, int height);
  ~OffscreenView() override;
  bool save(std::ostream& output) const;
  OffscreenContext *ctx;

  // overrides
  bool save(const char *filename) const override;
  [[nodiscard]] std::string getRendererInfo() const override;
#ifdef ENABLE_OPENCSG
  void display_opencsg_warning() override;
#endif
};
