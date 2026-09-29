/*
   Create a NULL OpenGL context that doesn't actually use any OpenGL code,
   and can be compiled and used on a system without OpenGL or GPU.
 */
#include "glview/OffscreenContextNULL.h"

#include <memory>
#include <string>

#include "glview/OffscreenContext.h"

class OffscreenContextNULL : public OffscreenContext
{
public:
  OffscreenContextNULL() : OffscreenContext(0, 0) {}
  ~OffscreenContextNULL() override = default;

  std::string getInfo() const override { return "GL context creator: NULLGL\n"; }

  bool makeCurrent() const override { return true; }
};

std::shared_ptr<OffscreenContext> CreateOffscreenContextNULL()
{
  return std::make_shared<OffscreenContextNULL>();
}
