#pragma once

#include "system-gl.h"
#include <stddef.h> // size_t

struct fbo_t
{
  GLuint fbo_id;
  GLuint old_fbo_id;

  GLuint renderbuf_id;
  GLuint depthbuf_id;
};

fbo_t *fbo_new();
bool fbo_init(fbo_t *fbo, size_t width, size_t height);
bool fbo_resize(fbo_t *fbo, size_t width, size_t height);
void fbo_delete(fbo_t *fbo);
GLuint fbo_bind(fbo_t *fbo);
void fbo_unbind(fbo_t *fbo);

bool REPORTGLERROR(const char *task);
