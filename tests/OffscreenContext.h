#ifndef OFFSCREENCONTEXT_H_
#define OFFSCREENCONTEXT_H_

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include <iostream>         // for error output

#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }

struct OffscreenContext *create_offscreen_context(int w, int h);
void bind_offscreen_context(OffscreenContext *ctx);
bool teardown_offscreen_context(OffscreenContext *ctx);
bool save_framebuffer(OffscreenContext *ctx, const char *filename);

#endif
