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
#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"
#include "system-gl.h"
#include "fbo.h"

#include <GL/gl.h>
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include <assert.h>
#include <sstream>
#include <string>

#include <sys/utsname.h> // for uname

struct OffscreenContext {

    OffscreenContext(int width, int height) : context(nullptr), display(nullptr), width(width), height(height), fbo(nullptr)
    {
    }

    EGLContext context;
    EGLDisplay display;
    int width;
    int height;
    fbo_t *fbo;
};

#include "OffscreenContextAll.hpp"

std::string get_os_info()
{
    struct utsname u;

    if (uname(&u) < 0) {
        return STR("OS info: unknown, uname() error\n");
    } else {
        return STR("OS info: " << u.sysname << " " << u.release << " " << u.version << "\n" <<
                "Machine: " << u.machine);
    }
    return "";
}

std::string get_gl_info(EGLDisplay display)
{
    char const * vendor = eglQueryString(display, EGL_VENDOR);
    char const * version = eglQueryString(display, EGL_VERSION);

    return STR("GL context creator: EGL\n"
            << "EGL version: " << version << " (" << vendor << ")" << "\n"
            << get_os_info());
}

std::string offscreen_context_getinfo(OffscreenContext *ctx)
{
    assert(ctx);

    if (!ctx->context) {
        return std::string("No GL Context initialized. No information to report\n");
    }

    return get_gl_info(ctx->display);
}

static bool create_egl_dummy_context(OffscreenContext &ctx)
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
        EGL_WIDTH, ctx.width,
        EGL_HEIGHT, ctx.height,
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
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 1,
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

OffscreenContext *create_offscreen_context(int w, int h)
{
    auto ctx = new OffscreenContext(w, h);

    if (!create_egl_dummy_context(*ctx)) {
        delete ctx;
        return nullptr;
    }

    typedef const GLubyte * (GLAPIENTRY * PFNGLGETSTRINGPROC) (GLenum name);
    PFNGLGETSTRINGPROC getString = (PFNGLGETSTRINGPROC) eglGetProcAddress("glGetString");
    PRINTDB("OpenGL Version: %s (%s)", getString(GL_VERSION) % getString(GL_VENDOR));

    return create_offscreen_context_common(ctx);
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
    if (ctx) {
        fbo_unbind(ctx->fbo);
        fbo_delete(ctx->fbo);
        eglTerminate(ctx->display);
        return true;
    }
    return false;
}

bool save_framebuffer(const OffscreenContext *ctx, std::ostream &output)
{
    return save_framebuffer_common(ctx, output);
}
