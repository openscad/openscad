/*
   Create an NULL OpenGL context that doesn't actually use any OpenGL code,
   and can be compiled on a system without OpenGL.
 */
#include "glview/OffscreenContextNULL.h"

#include <memory>
#include <string>

class OffscreenContextNULL : public OffscreenContext {
public:
  OffscreenContextNULL() : OffscreenContext(0, 0) {}
  ~OffscreenContextNULL() {}
  std::string getInfo() const override {
    return "GL context creator: NULLGL";
  }
  bool makeCurrent() const override { return true; }
};

std::shared_ptr<OffscreenContext> CreateOffscreenContextNULL()
{
  return std::make_shared<OffscreenContextNULL>();
}
