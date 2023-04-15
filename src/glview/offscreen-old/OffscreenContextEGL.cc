/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2021 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "OffscreenContextEGL.h"

#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include "system-gl.h"

#include <cassert>
#include <sstream>
#include <string>
#include <sys/utsname.h> // for uname

#include "printutils.h"
#include "imageutils.h"

class OffscreenContextEGL : public OffscreenContext {
public:
  OffscreenContextEGL(uint32_t width, uint32_t height) : OffscreenContext(width, height) {}
  ~OffscreenContextEGL() {
    if (this->display != nullptr) {
      eglTerminate(this->display);
    }
  }

  std::string getInfo() const override;
  
  EGLContext context{nullptr};
  EGLDisplay display{nullptr};
};

std::string get_gl_info(EGLDisplay display)
{
  std::ostringstream result;

  const char *vendor = eglQueryString(display, EGL_VENDOR);
  const char *version = eglQueryString(display, EGL_VERSION);

  result << "GL context creator: EGL\n"
	 << "EGL version: " << version << " (" << vendor << ")\n"
	 << "PNG generator: lodepng\n";

  return result.str();
}

std::string OffscreenContextEGL::getInfo() const {
  if (!this->context) {
    return {"No GL Context initialized. No information to report\n"};
  }

  return get_gl_info(this->display);
}

static bool create_egl_dummy_context(OffscreenContextEGL& ctx)
{
  static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_CONFORMANT, EGL_OPENGL_BIT,
    EGL_NONE
  };

  const EGLint pbufferAttribs[] = {
    EGL_WIDTH, ctx.width(),
    EGL_HEIGHT, ctx.height(),
    EGL_NONE,
  };

  EGLDisplay display = EGL_NO_DISPLAY;
  PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
    const int MAX_DEVICES = 10;
    EGLDeviceEXT eglDevs[MAX_DEVICES];
    EGLint numDevices = 0;

    eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);
    PRINTDB("Found %d EGL devices.", numDevices);
    for (int idx = 0; idx < numDevices; idx++) {
      EGLDisplay disp = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[idx], 0);
      if (disp != EGL_NO_DISPLAY) {
        display = disp;
        break;
      }
    }
  } else {
    PRINTD("Trying default EGL display...");
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }

  if (display == EGL_NO_DISPLAY) {
    PRINTD("No EGL display found.");
    return false;
  }

  PFNEGLGETDISPLAYDRIVERNAMEPROC eglGetDisplayDriverName = (PFNEGLGETDISPLAYDRIVERNAMEPROC) eglGetProcAddress("eglGetDisplayDriverName");
  if (eglGetDisplayDriverName) {
    const char *name = eglGetDisplayDriverName(display);
    PRINTDB("Got EGL display with driver name '%s'", name);
  }

  EGLint major, minor;
  if (!eglInitialize(display, &major, &minor)) {
    std::cerr << "Unable to initialize EGL" << std::endl;
    return false;
  }

  PRINTDB("EGL Version: %d.%d (%s)", major % minor % eglQueryString(display, EGL_VENDOR));

  EGLint numConfigs;
  EGLConfig config;
  if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
    std::cerr << "Failed to choose config (eglError: " << std::hex << eglGetError() << ")" << std::endl;
    return false;
  }
  if (!eglBindAPI(EGL_OPENGL_API)) {
    std::cerr << "Bind EGL_OPENGL_API failed!" << std::endl;
    return false;
  }
  EGLSurface surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
  if (surface == EGL_NO_SURFACE) {
    std::cerr << "Unable to create EGL surface (eglError: " << eglGetError() << ")" << std::endl;
    return false;
  }

  EGLint ctxattr[] = {
    EGL_CONTEXT_MAJOR_VERSION, 2,
    EGL_CONTEXT_MINOR_VERSION, 0,
    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
    EGL_NONE
  };
  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxattr);
  if (context == EGL_NO_CONTEXT) {
    std::cerr << "Unable to create EGL context (eglError: " << eglGetError() << ")" << std::endl;
    return 1;
  }

  eglMakeCurrent(display, surface, surface, context);
  glClearColor(1.0, 1.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  eglSwapBuffers(display, surface);
  ctx.display = display;
  ctx.context = context;
  return true;
}

std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(
    uint32_t width, uint32_t height, uint32_t majorGLVersion, 
    uint32_t minorGLVersion, bool gles, bool compatibilityProfile,
    const std::string& drmNode = "")
{
  auto ctx = std::make_shared<OffscreenContextEGL>(width, height);

  if (!create_egl_dummy_context(*ctx)) {
    return nullptr;
  }

  typedef const GLubyte *(GLAPIENTRY *PFNGLGETSTRINGPROC)(GLenum name);
  PFNGLGETSTRINGPROC getString = (PFNGLGETSTRINGPROC) eglGetProcAddress("glGetString");
  PRINTDB("OpenGL Version: %s (%s)", getString(GL_VERSION) % getString(GL_VENDOR));

  return ctx;
}
