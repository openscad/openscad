/*

Create an OpenGL context without creating an OpenGL Window. for Windows.

For more info:

   http://www.nullterminator.net/opengl32.html by Blaine Hodge 
   http://msdn.microsoft.com/en-us/library/ee418815(v=vs.85).aspx
   http://www.cprogramming.com/tutorial/wgl_wiggle_functions.html by RoD
    ( which includes robot.cc by Steven Billington )
   http://blogs.msdn.com/b/oldnewthing/archive/2006/12/04/1205831.aspx by Tom
*/

#undef NOGDI
#include <windows.h>
#include <vector>

#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"
#include "system-gl.h"
#include "fbo.h"

#include <GL/gl.h> // must be included after glew.h

#include <map>
#include <string>
#include <sstream>

using namespace std;

struct OffscreenContext
{
  HWND window;
  HDC dev_context;
  HGLRC openGLContext;
  int width;
  int height;
  fbo_t *fbo;
};

#include "OffscreenContextAll.hpp"

void offscreen_context_init(OffscreenContext &ctx, int width, int height)
{
  ctx.window = (HWND)nullptr;
  ctx.dev_context = (HDC)nullptr;
  ctx.openGLContext = (HGLRC)nullptr;
  ctx.width = width;
  ctx.height = height;
  ctx.fbo = nullptr;
}

string get_os_info()
{
  OSVERSIONINFO osvi;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  SYSTEM_INFO si;
  GetSystemInfo(&si);
  map<WORD,const char*> archs;
  archs[PROCESSOR_ARCHITECTURE_AMD64] = "amd64";
  archs[PROCESSOR_ARCHITECTURE_IA64] = "itanium";
  archs[PROCESSOR_ARCHITECTURE_INTEL] = "x86";
  archs[PROCESSOR_ARCHITECTURE_UNKNOWN] = "unknown";

  stringstream out;
  out << "OS info: "
      << "Microsoft(TM) Windows(TM) " << osvi.dwMajorVersion << " "
      << osvi.dwMinorVersion << " " << osvi.dwBuildNumber << " "
      << osvi.szCSDVersion;
  if (archs.find(si.wProcessorArchitecture) != archs.end()) 
    out << " " << archs[si.wProcessorArchitecture];
  out << "\n";

  out << "Machine: " << si.dwProcessorType;

  return out.str();
}

string offscreen_context_getinfo(OffscreenContext * /*ctx*/)
{
  // should probably get some info from WGL context here?
  stringstream out;
  out << "GL context creator: WGL\n"
      << "PNG generator: lodepng\n"
      << get_os_info();
  return out.str();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) 
{
  return DefWindowProc( hwnd, message, wparam, lparam );
}

bool create_wgl_dummy_context(OffscreenContext &ctx)
{
  // this function alters ctx->window and ctx->openGLContext 
  //  and ctx->dev_context if successfull

  // create window

  HINSTANCE inst = GetModuleHandle(0);
  WNDCLASS wc;
  ZeroMemory( &wc, sizeof( wc ) );
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = inst;
  wc.lpszClassName = L"OpenSCAD";
  ATOM class_atom = RegisterClassW( &wc );

  if ( class_atom == 0 ) {
    cerr << "MS GDI - RegisterClass failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  LPCTSTR lpClassName = L"OpenSCAD";
  LPCTSTR lpWindowName = L"OpenSCAD";
  DWORD dwStyle = WS_CAPTION | WS_POPUPWINDOW; // | WS_VISIBLE
  int x = 0;
  int y = 0;
  int nWidth = ctx.width;
  int nHeight = ctx.height;
  HWND hWndParent = nullptr;
  HMENU hMenu = nullptr;
  HINSTANCE hInstance = inst;
  LPVOID lpParam = nullptr;

  HWND window = CreateWindowW( lpClassName, lpWindowName, dwStyle, x, y,
    nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam );

  if ( window==nullptr ) {
    cerr << "MS GDI - CreateWindow failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  // create WGL context, make current

  PIXELFORMATDESCRIPTOR pixformat;
  int chosenformat;
  HDC dev_context = GetDC( window );
  if ( dev_context == nullptr ) {
    cerr << "MS GDI - GetDC failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  ZeroMemory( &pixformat, sizeof( pixformat ) );
  pixformat.nSize = sizeof( pixformat );
  pixformat.nVersion = 1;
  pixformat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pixformat.iPixelType = PFD_TYPE_RGBA;
  pixformat.cGreenBits = 8;
  pixformat.cRedBits = 8;
  pixformat.cBlueBits = 8;
  pixformat.cAlphaBits = 8;
  pixformat.cDepthBits = 24;
  pixformat.cStencilBits = 8;

  chosenformat = ChoosePixelFormat( dev_context, &pixformat );
  if (chosenformat==0) {
    cerr << "MS GDI - ChoosePixelFormat failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  bool spfok = SetPixelFormat( dev_context, chosenformat, &pixformat );
  if (!spfok) {
    cerr << "MS GDI - SetPixelFormat failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  HGLRC gl_render_context = wglCreateContext( dev_context );
  if ( gl_render_context == nullptr ) {
      cerr << "MS WGL - wglCreateContext failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
      ReleaseDC( ctx.window, ctx.dev_context );
      return false;
  }

  bool mcok = wglMakeCurrent( dev_context, gl_render_context );
  if (!mcok) {
    cerr << "MS WGL - wglMakeCurrent failed\n";
    cerr << "last-error code: " << GetLastError() << "\n";
    return false;
  }

  ctx.window = window;
  ctx.dev_context = dev_context;
  ctx.openGLContext = gl_render_context;

  return true;
}


OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  offscreen_context_init( *ctx, w, h );

  // Before an FBO can be setup, a WGL context must be created. 
  // This call alters ctx->window and ctx->openGLContext 
  //  and ctx->dev_context if successfull
  if (!create_wgl_dummy_context( *ctx )) {
    return nullptr;
  }

  return create_offscreen_context_common( ctx );
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  if (ctx) {
    fbo_unbind(ctx->fbo);
    fbo_delete(ctx->fbo);

    wglMakeCurrent( nullptr, nullptr );
    wglDeleteContext( ctx->openGLContext );
    ReleaseDC( ctx->window, ctx->dev_context );

    return true;
  }
  return false;
}

bool save_framebuffer(OffscreenContext *ctx, std::ostream &output)
{
  if (!ctx) return false;
  wglSwapLayerBuffers( ctx->dev_context, WGL_SWAP_MAIN_PLANE );
  return save_framebuffer_common( ctx, output );
}

