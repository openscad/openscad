/*

   Create an OpenGL context without creating an OpenGL Window. for Linux.

   See also

   glxgears.c by Brian Paul from mesa-demos (mesa3d.org)
   http://cgit.freedesktop.org/mesa/demos/tree/src/xdemos?id=mesa-demos-8.0.1
   http://www.opengl.org/sdk/docs/man/xhtml/glXIntro.xml
   http://www.mesa3d.org/brianp/sig97/offscrn.htm
   http://glprogramming.com/blue/ch07.html
   OffscreenContext.mm (Mac OSX version)

 */

/*
 * Some portions of the code below are:
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"
#include "system-gl.h"
#include "fbo.h"

#include <GL/gl.h>
#include <GL/glx.h>

#include <assert.h>
#include <sstream>
#include <string>

#include <sys/utsname.h> // for uname

struct OffscreenContext
{
	OffscreenContext(int width, int height) :
		openGLContext(nullptr), xdisplay(nullptr), xwindow(0),
		width(width), height(height),
		fbo(nullptr) {}
	GLXContext openGLContext;
	Display *xdisplay;
	Window xwindow;
	int width;
	int height;
	fbo_t *fbo;
};

#include "OffscreenContextAll.hpp"

std::string get_os_info()
{
	struct utsname u;
	std::stringstream out;

	if (uname(&u) < 0) {
		out << "OS info: unknown, uname() error\n";
	}
	else {
		out << "OS info: "
				<< u.sysname << " "
				<< u.release << " "
				<< u.version << "\n";
		out << "Machine: " << u.machine;
	}
	return out.str();
}

std::string offscreen_context_getinfo(OffscreenContext *ctx)
{
	assert(ctx);

	if (!ctx->xdisplay) {
		return std::string("No GL Context initialized. No information to report\n");
	}

	int major, minor;
	glXQueryVersion(ctx->xdisplay, &major, &minor);

	std::stringstream out;
	out << "GL context creator: GLX\n"
			<< "PNG generator: lodepng\n"
			<< "GLX version: " << major << "." << minor << "\n"
			<< get_os_info();

	return out.str();
}

static XErrorHandler original_xlib_handler = nullptr;
static auto XCreateWindow_failed = false;
static int XCreateWindow_error(Display *dpy, XErrorEvent *event)
{
	std::cerr << "XCreateWindow failed: XID: " << event->resourceid
						<< " request: " << static_cast<int>(event->request_code)
						<< " minor: " << static_cast<int>(event->minor_code) << "\n";
	char description[1024];
	XGetErrorText(dpy, event->error_code, description, 1023);
	std::cerr << " error message: " << description << "\n";
	XCreateWindow_failed = true;
	return 0;
}

/*
   create a dummy X window without showing it. (without 'mapping' it)
   and save information to the ctx.

   This purposely does not use glxCreateWindow, to avoid crashes,
   "failed to create drawable" errors, and Mesa "WARNING: Application calling
   GLX 1.3 function when GLX 1.3 is not supported! This is an application bug!"

   This function will alter ctx.openGLContext and ctx.xwindow if successfull
 */
bool create_glx_dummy_window(OffscreenContext &ctx)
{
	int attributes[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT, //support all 3, for OpenCSG
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24, // depth-stencil for OpenCSG
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER, true,
		None
	};

	auto dpy = ctx.xdisplay;

	int num_returned = 0;
	auto fbconfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attributes, &num_returned);
	if (fbconfigs == nullptr) {
		std::cerr << "glXChooseFBConfig failed\n";
		return false;
	}

	auto visinfo = glXGetVisualFromFBConfig(dpy, fbconfigs[0]);
	if (visinfo == nullptr) {
		std::cerr << "glXGetVisualFromFBConfig failed\n";
		XFree(fbconfigs);
		return false;
	}

	// can't depend on xWin==nullptr at failure. use a custom Xlib error handler instead.
	original_xlib_handler = XSetErrorHandler(XCreateWindow_error);

	auto root = DefaultRootWindow(dpy);
	XSetWindowAttributes xwin_attr;
	auto width = ctx.width;
	auto height = ctx.height;
	xwin_attr.background_pixmap = None;
	xwin_attr.background_pixel = 0;
	xwin_attr.border_pixel = 0;
	xwin_attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	xwin_attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	unsigned long int mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	auto xWin = XCreateWindow(dpy, root, 0, 0, width, height,
														0, visinfo->depth, InputOutput,
														visinfo->visual, mask, &xwin_attr);

	// Window xWin = XCreateSimpleWindow( dpy, DefaultRootWindow(dpy), 0,0,42,42, 0,0,0 );

	XSync(dpy, false);
	if (XCreateWindow_failed) {
		XFree(visinfo);
		XFree(fbconfigs);
		return false;
	}
	XSetErrorHandler(original_xlib_handler);

	// Most programs would call XMapWindow here. But we don't, to keep the window hidden
	// XMapWindow( dpy, xWin );

	auto context = glXCreateNewContext(dpy, fbconfigs[0], GLX_RGBA_TYPE, nullptr, true);
	if (context == nullptr) {
		std::cerr << "glXCreateNewContext failed\n";
		XDestroyWindow(dpy, xWin);
		XFree(visinfo);
		XFree(fbconfigs);
		return false;
	}

	//GLXWindow glxWin = glXCreateWindow( dpy, fbconfigs[0], xWin, nullptr );

	if (!glXMakeContextCurrent(dpy, xWin, xWin, context)) {
		//if (!glXMakeContextCurrent( dpy, glxWin, glxWin, context )) {
		std::cerr << "glXMakeContextCurrent failed\n";
		glXDestroyContext(dpy, context);
		XDestroyWindow(dpy, xWin);
		XFree(visinfo);
		XFree(fbconfigs);
		return false;
	}

	ctx.openGLContext = context;
	ctx.xwindow = xWin;

	XFree(visinfo);
	XFree(fbconfigs);

	return true;
}

bool create_glx_dummy_context(OffscreenContext &ctx);

OffscreenContext *create_offscreen_context(int w, int h)
{
	auto ctx = new OffscreenContext(w, h);

	// before an FBO can be setup, a GLX context must be created
	// this call alters ctx->xDisplay and ctx->openGLContext
	// and ctx->xwindow if successful
	if (!create_glx_dummy_context(*ctx)) {
		delete ctx;
		return nullptr;
	}

	return create_offscreen_context_common(ctx);
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
	if (ctx) {
		fbo_unbind(ctx->fbo);
		fbo_delete(ctx->fbo);
		XDestroyWindow(ctx->xdisplay, ctx->xwindow);
		glXDestroyContext(ctx->xdisplay, ctx->openGLContext);
		XCloseDisplay(ctx->xdisplay);
		return true;
	}
	return false;
}

bool save_framebuffer(OffscreenContext *ctx, std::ostream &output)
{
	glXSwapBuffers(ctx->xdisplay, ctx->xwindow);
	return save_framebuffer_common(ctx, output);
}

#pragma GCC diagnostic ignored "-Waddress"
bool create_glx_dummy_context(OffscreenContext &ctx)
{
	// This will alter ctx.openGLContext and ctx.xdisplay and ctx.xwindow if successfull
	int major;
	int minor;
	auto result = false;

	ctx.xdisplay = XOpenDisplay(nullptr);
	if (ctx.xdisplay == nullptr) {
		std::cerr << "Unable to open a connection to the X server.\n";
		auto dpyenv = getenv("DISPLAY");
		std::cerr << "DISPLAY=" << (dpyenv ? dpyenv : "") << "\n";
		return false;
	}

	// glxQueryVersion is not always reliable. Use it, but then
	// also check to see if GLX 1.3 functions exist

	glXQueryVersion(ctx.xdisplay, &major, &minor);
	if (major == 1 && minor <= 2 && glXGetVisualFromFBConfig == nullptr) {
		std::cerr << "Error: GLX version 1.3 functions missing. "
							<< "Your GLX version: " << major << "." << minor << std::endl;
	}
	else {
		result = create_glx_dummy_window(ctx);
	}

	if (!result) XCloseDisplay(ctx.xdisplay);
	return result;
}
