/*

   Create an OpenGL context without creating an OpenGL Window. for Windows.

   For more info:

   http://www.nullterminator.net/opengl32.html by Blaine Hodge
   http://msdn.microsoft.com/en-us/library/ee418815(v=vs.85).aspx
   http://www.cprogramming.com/tutorial/wgl_wiggle_functions.html by RoD
    ( which includes robot.cc by Steven Billington )
   http://blogs.msdn.com/b/oldnewthing/archive/2006/12/04/1205831.aspx by Tom
 */
#include "glview/offscreen-old/OffscreenContextWGL.h"

#undef NOGDI
#include <iostream>
#include <cstdint>
#include <memory>
#include <windows.h>

#include <string>
#include <sstream>
#include "utils/printutils.h"
#include "glview/system-gl.h"
#include <GL/gl.h> // must be included after glew.h


namespace {

class OffscreenContextWGL : public OffscreenContext {
public:
  OffscreenContextWGL(uint32_t width, uint32_t height) : OffscreenContext(width, height) {}
  ~OffscreenContextWGL() {
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(this->openGLContext);
    ReleaseDC(this->window, this->dev_context);
  }

  std::string getInfo() const override {
  std::stringstream result;
  // should probably get some info from WGL context here?
  result << "GL context creator: WGL (old)\n"
      	 << "PNG generator: lodepng\n";

  return result.str();
}

  bool makeCurrent() const override {
    return wglMakeCurrent(this->dev_context, this->openGLContext);
  }

  HWND window{nullptr};
  HDC dev_context{nullptr};
  HGLRC openGLContext{nullptr};
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  return DefWindowProc(hwnd, message, wparam, lparam);
}

bool create_wgl_dummy_context(OffscreenContextWGL& ctx)
{
  // this function alters ctx->window and ctx->openGLContext
  //  and ctx->dev_context if successful

  // create window
  LPCWSTR lpClassName = L"OpenSCAD";

  HINSTANCE inst = GetModuleHandleW(0);
  WNDCLASSW wc;
  ZeroMemory(&wc, sizeof(wc) );
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = inst;
  wc.lpszClassName = lpClassName;
  ATOM class_atom = RegisterClassW(&wc);

  if (class_atom == 0) {
    const DWORD last_error = GetLastError();
    if (last_error != ERROR_CLASS_ALREADY_EXISTS) {
      std::cerr << "MS GDI - RegisterClass failed\n";
      std::cerr << "last-error code: " << last_error << "\n";
      return false;
    }
  }

  LPCWSTR lpWindowName = L"OpenSCAD";
  DWORD dwStyle = WS_CAPTION | WS_POPUPWINDOW; // | WS_VISIBLE
  int x = 0;
  int y = 0;
  int nWidth = ctx.width();
  int nHeight = ctx.height();
  HWND hWndParent = nullptr;
  HMENU hMenu = nullptr;
  HINSTANCE hInstance = inst;
  LPVOID lpParam = nullptr;

  HWND window = CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y,
                              nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

  if (window == nullptr) {
    std::cerr << "MS GDI - CreateWindow failed\n";
    std::cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  // create WGL context, make current

  PIXELFORMATDESCRIPTOR pixformat;
  int chosenformat;
  HDC dev_context = GetDC(window);
  if (dev_context == nullptr) {
    std::cerr << "MS GDI - GetDC failed\n";
    std::cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  ZeroMemory(&pixformat, sizeof(pixformat) );
  pixformat.nSize = sizeof(pixformat);
  pixformat.nVersion = 1;
  pixformat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pixformat.iPixelType = PFD_TYPE_RGBA;
  pixformat.cGreenBits = 8;
  pixformat.cRedBits = 8;
  pixformat.cBlueBits = 8;
  pixformat.cAlphaBits = 8;
  pixformat.cDepthBits = 24;
  pixformat.cStencilBits = 8;

  chosenformat = ChoosePixelFormat(dev_context, &pixformat);
  if (chosenformat == 0) {
    std::cerr << "MS GDI - ChoosePixelFormat failed\n";
    std::cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  bool spfok = SetPixelFormat(dev_context, chosenformat, &pixformat);
  if (!spfok) {
    std::cerr << "MS GDI - SetPixelFormat failed\n";
    std::cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  HGLRC gl_render_context = wglCreateContext(dev_context);
  if (gl_render_context == nullptr) {
    std::cerr << "MS WGL - wglCreateContext failed\n";
    std::cerr << "last-error code: " << GetLastError() << "\n";
    ReleaseDC(ctx.window, ctx.dev_context);
    return false;
  }

  ctx.window = window;
  ctx.dev_context = dev_context;
  ctx.openGLContext = gl_render_context;

  return true;
}

}  // namespace

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextWGL(
  uint32_t width, uint32_t height, uint32_t majorGLVersion, 
  uint32_t minorGLVersion, bool compatibilityProfile)   
{
  auto ctx = std::make_shared<OffscreenContextWGL>(width, height);

  // Before an FBO can be setup, a WGL context must be created.
  // This call alters ctx->window and ctx->openGLContext
  //  and ctx->dev_context if successful
  if (!create_wgl_dummy_context(*ctx)) {
    return nullptr;
  }

  return ctx;
}

}  // namespace offscreen_old
