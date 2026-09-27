#include "glview/OffscreenContextOSMesa.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef GLAPI
#define GLAPI extern
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif
#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif

#include <GL/osmesa.h>

#include "utils/printutils.h"

class OffscreenContextOSMesa : public OffscreenContext
{
public:
  OSMesaContext osmesaContext = nullptr;
  mutable std::vector<uint8_t> buffer;

  OffscreenContextOSMesa(uint32_t width, uint32_t height)
    : OffscreenContext(width, height), buffer(width * height * 4, 0)
  {
  }

  ~OffscreenContextOSMesa() override
  {
    if (osmesaContext) {
      OSMesaDestroyContext(osmesaContext);
      osmesaContext = nullptr;
    }
  }

  std::string getInfo() const override
  {
    std::ostringstream out;
    out << "GL context creator: OSMesa\n";
    return out.str();
  }

  bool makeCurrent() const override
  {
    if (!osmesaContext) {
      LOG(message_group::Error, "OSMesa context is null");
      return false;
    }
    if (!OSMesaMakeCurrent(osmesaContext, buffer.data(), GL_UNSIGNED_BYTE, width_, height_)) {
      LOG(message_group::Error, "OSMesaMakeCurrent() failed");
      return false;
    }
    return true;
  }
};

std::shared_ptr<OffscreenContext> CreateOffscreenContextOSMesa(uint32_t width, uint32_t height,
                                                               uint32_t majorGLVersion,
                                                               uint32_t minorGLVersion,
                                                               bool compatibilityProfile)
{
  auto ctx = std::make_shared<OffscreenContextOSMesa>(width, height);

  std::vector<int> attribs;
  attribs.push_back(OSMESA_FORMAT);
  attribs.push_back(OSMESA_RGBA);
  attribs.push_back(OSMESA_DEPTH_BITS);
  attribs.push_back(24);
  attribs.push_back(OSMESA_STENCIL_BITS);
  attribs.push_back(8);
  attribs.push_back(OSMESA_ACCUM_BITS);
  attribs.push_back(0);

  if (majorGLVersion >= 3) {
    attribs.push_back(OSMESA_PROFILE);
    attribs.push_back(compatibilityProfile ? OSMESA_COMPAT_PROFILE : OSMESA_CORE_PROFILE);
    attribs.push_back(OSMESA_CONTEXT_MAJOR_VERSION);
    attribs.push_back(static_cast<int>(majorGLVersion));
    attribs.push_back(OSMESA_CONTEXT_MINOR_VERSION);
    attribs.push_back(static_cast<int>(minorGLVersion));
  }
  attribs.push_back(0);

  ctx->osmesaContext = OSMesaCreateContextAttribs(attribs.data(), nullptr);
  if (!ctx->osmesaContext && majorGLVersion <= 2) {
    // Fallback for OpenGL 2.x
    ctx->osmesaContext = OSMesaCreateContextExt(OSMESA_RGBA, 24, 8, 0, nullptr);
  }

  if (!ctx->osmesaContext) {
    LOG(message_group::Error, "OSMesaCreateContext failed for GL %1$d.%2$d", majorGLVersion,
        minorGLVersion);
    return nullptr;
  }

  return ctx;
}
