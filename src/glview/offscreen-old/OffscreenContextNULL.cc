/*

   Create an NULL OpenGL context that doesn't actually use any OpenGL code,
   and can be compiled on a system without OpenGL.

 */

#include <vector>

#include "OffscreenContext.h"
#include "printutils.h"

#include <map>
#include <string>
#include <sstream>

using namespace std;

class OffscreenContextNULL : public OffscreenContext {
public:
  OffscreenContextNULL() : OffscreenContext(0, 0) {}
  std::string getInfo() const override;
};

string offscreen_context_getinfo(OffscreenContext *ctx)
{
  return "";
}

std::shared_ptr<OffscreenContext> CreateOffscreenContextNULL()
{
  auto ctx = std::make_shared<OffscreenContextNULL>();
  return ctx;
}
