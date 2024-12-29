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
#include "glview/offscreen-old/OffscreenContextEGL.h"

#include <iostream>
#include <cstdint>
#include <memory>
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include "glview/system-gl.h"

#include <cassert>
#include <sstream>
#include <string>
#include <sys/utsname.h> // for uname

#include "glview/OffscreenContext.h"

namespace {

class OffscreenContextEGL : public OffscreenContext {
public:
  OffscreenContextEGL(uint32_t width, uint32_t height) : OffscreenContext(width, height) {}
  ~OffscreenContextEGL() {
    if (this->display != nullptr) {
      eglTerminate(this->display);
    }
  }

  std::string getInfo() const override {
    if (!this->context) {
      return {"No GL Context initialized. No information to report\n"};
    }

    std::ostringstream result;

    const char *vendor = eglQueryString(display, EGL_VENDOR);
    const char *version = eglQueryString(display, EGL_VERSION);

    result << "GL context creator: EGL (old)\n"
    << "EGL version: " << version << " (" << vendor << ")\n"
    << "PNG generator: lodepng\n";

    return result.str();
  }
  
  bool makeCurrent() const override {
    return eglMakeCurrent(this->display, this->surface, this->surface, this->context);
  }

  EGLContext context{nullptr};
  EGLDisplay display{nullptr};
  EGLSurface surface{EGL_NO_SURFACE};
};

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
    EGL_WIDTH, static_cast<EGLint>(ctx.width()),
    EGL_HEIGHT, static_cast<EGLint>(ctx.height()),
    EGL_NONE,
  };

  PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
    const int MAX_DEVICES = 10;
    EGLDeviceEXT eglDevs[MAX_DEVICES];
    EGLint numDevices = 0;

    eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);
    PRINTDB("Found %d EGL devices.", numDevices);
    for (int idx = 0; idx < numDevices; idx++) {
      EGLDisplay disp = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[idx], nullptr);
      if (disp != EGL_NO_DISPLAY) {
        ctx.display = disp;
        break;
      }
    }
  } else {
    PRINTD("Trying default EGL display...");
    ctx.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }

  if (ctx.display == EGL_NO_DISPLAY) {
    PRINTD("No EGL display found.");
    return false;
  }

  PFNEGLGETDISPLAYDRIVERNAMEPROC eglGetDisplayDriverName = (PFNEGLGETDISPLAYDRIVERNAMEPROC) eglGetProcAddress("eglGetDisplayDriverName");
  if (eglGetDisplayDriverName) {
    const char *name = eglGetDisplayDriverName(ctx.display);
    PRINTDB("Got EGL display with driver name '%s'", name);
  }

  EGLint major, minor;
  if (!eglInitialize(ctx.display, &major, &minor)) {
    std::cerr << "Unable to initialize EGL" << std::endl;
    return false;
  }

  PRINTDB("EGL Version: %d.%d (%s)", major % minor % eglQueryString(ctx.display, EGL_VENDOR));

  EGLint numConfigs;
  EGLConfig config;
  if (!eglChooseConfig(ctx.display, configAttribs, &config, 1, &numConfigs)) {
    std::cerr << "Failed to choose config (eglError: " << std::hex << eglGetError() << ")" << std::endl;
    return false;
  }
  if (!eglBindAPI(EGL_OPENGL_API)) {
    std::cerr << "Bind EGL_OPENGL_API failed!" << std::endl;
    return false;
  }
  ctx.surface = eglCreatePbufferSurface(ctx.display, config, pbufferAttribs);
  if (ctx.surface == EGL_NO_SURFACE) {
    std::cerr << "Unable to create EGL surface (eglError: " << eglGetError() << ")" << std::endl;
    return false;
  }

  EGLint ctxattr[] = {
    EGL_CONTEXT_MAJOR_VERSION, 2,
    EGL_CONTEXT_MINOR_VERSION, 0,
    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
    EGL_NONE
  };
  ctx.context = eglCreateContext(ctx.display, config, EGL_NO_CONTEXT, ctxattr);
  if (ctx.context == EGL_NO_CONTEXT) {
    std::cerr << "Unable to create EGL context (eglError: " << eglGetError() << ")" << std::endl;
    return false;
  }

  return true;
}

}  // namespace

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(
    uint32_t width, uint32_t height, uint32_t majorGLVersion, 
    uint32_t minorGLVersion, bool gles, bool compatibilityProfile)
{
  auto ctx = std::make_shared<OffscreenContextEGL>(width, height);

  if (!create_egl_dummy_context(*ctx)) {
    return nullptr;
  }
  return ctx;
}

}  // namespace offscreen_old
