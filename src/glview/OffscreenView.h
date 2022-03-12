#pragma once

#include "OffscreenContext.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include "system-gl.h"
#include <iostream>
#include "GLView.h"

class OffscreenView : public GLView
{
public:
  OffscreenView(int width, int height);
  ~OffscreenView();
  bool save(std::ostream& output) const;
  OffscreenContext *ctx;

  // overrides
  bool save(const char *filename) const override;
  std::string getRendererInfo() const override;
#ifdef ENABLE_OPENCSG
  void display_opencsg_warning() override;
#endif
};
