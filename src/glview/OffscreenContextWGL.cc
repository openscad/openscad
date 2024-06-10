#include "OffscreenContextWGL.h"

#include <iostream>
#include <sstream>

#undef NOGDI // To access ChoosePixelFormat, PIXELFORMATDESCRIPTOR, etc.
#include <windows.h>
#include <wingdi.h>
#define GLAD_WGL
#define GLAD_WGL_IMPLEMENTATION
#include <glad/wgl.h>

#include "printutils.h"
#include "scope_guard.hpp"

class OffscreenContextWGL : public OffscreenContext {

public:
  HWND window = nullptr;
  HDC deviceContext = nullptr;
  HGLRC renderContext = nullptr;

  OffscreenContextWGL(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextWGL() {
    wglMakeCurrent(nullptr, nullptr);
    if (this->renderContext) wglDeleteContext(this->renderContext);
    if (this->deviceContext) ReleaseDC(this->window, this->deviceContext);
    if (this->window) DestroyWindow(this->window);
  }

  std::string getInfo() const override {
    std::stringstream result;
    // should probably get some info from WGL context here?
    result << "GL context creator: WGL (new)\n"
          << "PNG generator: lodepng\n";

    return result.str();
  }

  bool makeCurrent() const override {
    return wglMakeCurrent(this->deviceContext, this->renderContext);
  }
};

std::shared_ptr<OffscreenContext> CreateOffscreenContextWGL(size_t width, size_t height,
							    size_t majorGLVersion, size_t minorGLVersion, bool compatibilityProfile)
{
  auto ctx = std::make_shared<OffscreenContextWGL>(width, height);

  WNDCLASSEX wndClass = {
    .cbSize = sizeof(WNDCLASSEX),
    .style = CS_OWNDC,
    .lpfnWndProc = &DefWindowProc,
    .lpszClassName = L"OpenSCAD"
  };
  static ATOM atom = RegisterClassEx(&wndClass);

  // Create the window. Position and size it.
  // Style the window and remove the caption bar (WS_POPUP)
  ctx->window = CreateWindowEx(0, MAKEINTATOM(atom), L"openscad", WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
    CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, 0, 0);
  ctx->deviceContext = GetDC(ctx->window);
  if (ctx->deviceContext == nullptr) {
    std::cerr << "GetDC() failed: " << std::system_category().message(GetLastError()) << std::endl;
    return nullptr;
  }

  PIXELFORMATDESCRIPTOR pixelFormatDesc = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
    .iPixelType = PFD_TYPE_RGBA,

    .cColorBits = 32,
    .cDepthBits = 24,
    .cStencilBits = 8
  };
  int pixelFormat = ChoosePixelFormat(ctx->deviceContext, &pixelFormatDesc);
  if (!pixelFormat) {
    std::cerr << "ChoosePixelFormat() failed: " << std::system_category().message(GetLastError()) << std::endl;
    return nullptr;
  }

  if (!SetPixelFormat(ctx->deviceContext, pixelFormat, &pixelFormatDesc)) {
    std::cerr << "SetPixelFormat() failed: " << std::system_category().message(GetLastError()) << std::endl;
    return nullptr;
  }

  const auto tmpRenderContext = wglCreateContext(ctx->deviceContext);
  if (tmpRenderContext == nullptr) {
    std::cerr << "wglCreateContext() failed: " << std::system_category().message(GetLastError()) << std::endl;
    return nullptr;
  }
  auto guard = sg::make_scope_guard([tmpRenderContext]() {
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(tmpRenderContext);
  });

  if (!wglMakeCurrent(ctx->deviceContext, tmpRenderContext)) {
    std::cerr << "wglMakeCurrent() failed: " << std::system_category().message(GetLastError()) << std::endl;
    return nullptr;
  }

  auto wglVersion = gladLoaderLoadWGL(ctx->deviceContext);
  if (wglVersion == 0) {
    std::cerr << "GLAD: Unable to load WGL" << std::endl;
    return nullptr;
  }
  PRINTDB("GLAD: Loaded WGL %d.%d", GLAD_VERSION_MAJOR(wglVersion) % GLAD_VERSION_MINOR(wglVersion));

  if (!wglCreateContextAttribsARB) {
    std::cerr << "wglCreateContextAttribsARB() not available" << std::endl;
    return nullptr;
  }

  // Note: If we want to use wglChoosePixelFormatARB() to request a special pixel format, we could do that here.

  int attributes[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, static_cast<int>(majorGLVersion),
    WGL_CONTEXT_MINOR_VERSION_ARB, static_cast<int>(minorGLVersion),
    WGL_CONTEXT_PROFILE_MASK_ARB,
    compatibilityProfile ? WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB : WGL_CONTEXT_CORE_PROFILE_BIT_ARB,         
    0
  };  
  ctx->renderContext = wglCreateContextAttribsARB(ctx->deviceContext, nullptr, attributes);
  if (ctx->renderContext == nullptr) {
    std::cerr << "wglCreateContextAttribsARB() failed: " << std::system_category().message(GetLastError()) << std::endl;
    return nullptr;
  }

  return ctx;
}
