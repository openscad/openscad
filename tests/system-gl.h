#ifndef SYSTEMGL_H_
#define SYSTEMGL_H_

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>      // for gluCheckExtension
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }

#endif
