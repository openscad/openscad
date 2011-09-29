#ifndef FBOUTILS_H_
#define FBOUTILS_H_

#include "system-gl.h"

GLuint fbo_create(GLsizei width, GLsizei height);
void fbo_bind(GLuint fbo);
void fbo_unbind();

#endif
