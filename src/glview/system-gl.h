#pragma once

#ifndef NULLGL

#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
#include <GL/glew.h>
#endif
#ifdef USE_GLAD
  #ifdef _WIN32
  #define NORESOURCE // To avoid picking up DIFFERENCE from winuser.h, conflicting with OpenSCADOperator::DIFFERENCE
  #include <windows.h>
  #endif
#include "glad/gl.h"
#endif

#ifdef __APPLE__
 #include <OpenGL/glu.h>
#else
 #include <GL/glu.h>
#endif

#include <string>
#include "utils/printutils.h"

namespace {

// Returns true on OK, false on error
bool glCheck(const char *stmt, const char *file, int line)
{
  if (const auto err = glGetError(); err != GL_NO_ERROR) {
    LOG(message_group::Error, Location::NONE, "",
        "OpenGL error: %1$s (0x%2$04x) in %3$s:%4$d\n"
        "              %5$s\n", gluErrorString(err), err, file, line, stmt);
    return false;
  }
  return true;
}

// Returns true on OK, false on error
bool glCheckd(const char *stmt, const char *file, int line)
{
  if (const auto err = glGetError(); err != GL_NO_ERROR) {
    PRINTDB("OpenGL error: %s (0x%04x) in %s:%d\n"
            "              %s\n", gluErrorString(err) % err % file % line % stmt);
    return false;
  }
  return true;
}

} // namespace

// We have 4 different GL checks:
// GL_CHECK(statement);
//   Outputs LOG(Error) in both release and debug mode.
//   Always prints the output on error.
// IF_GL_CHECK(statement) then_statement;
//   Outputs LOG(Error) in both release and debug mode.
//   Always prints the output on error.
//   Executes then_statement on error.
// GL_CHECKD(statement);
//   Outputs PRINTDB() in both release and debug mode.
//   Prints the output on error only if --debug is used
// GL_DEBUG_CHECKD(statement)
//   Outputs PRINTDB() in debug mode only.
//   This is fast in release mode (executes statement only).

// GL_CHECK(statement);
// Use this for important error output causes output on error, also in release mode.
//
// This example will print an error if glClear() fails:
// GL_CHECK(glClear());
#define GL_CHECK(stmt) stmt; glCheck(#stmt, __FILE__, __LINE__)

// IF_GL_CHECK(statement) then_statement;
// Use this for important error handling which always causes an error, also in release mode
//
// This example will print an error and return false if glClear() fails:
// IF_GL_CHECK(glClear()) return false;
#define IF_GL_CHECK(stmt) stmt; if (!glCheck(#stmt, __FILE__, __LINE__))

// GL_CHECKD(statement);
// Use this for OpenGL debug error output which should make it into the release build
// Enable debug output at runtime using --enable=
// Note: This always checks glGetError(), so it will have performance implications.
//
// This example will print an error if glClear() fails, and if --debug is specified:
// GL_CHECKD(glClear());
#define GL_CHECKD(stmt) stmt; glCheckd(#stmt, __FILE__, __LINE__)

// GL_DEBUG_CHECKD(statement)
// Use this for OpenGL debug output which needs to be fast in release mode
// Debug output is only available for debug build and only when using --enable= at runtime
//
// This example will print an error if glClear() fails if --debug is specified, but yields just glClear in release mode.
// GL_DEBUG_CHECKD(glClear());
#ifdef DEBUG
#define GL_DEBUG_CHECKD(stmt) stmt; glCheckd(#stmt, __FILE__, __LINE__)
#else
#define GL_DEBUG_CHECKD(stmt) stmt
#endif

#else // NULLGL

#define GLint int
#define GLuint unsigned int
#define GLdouble unsigned int
inline void glColor4fv(float *c) {}

#endif // NULLGL

#ifdef USE_GLEW
#define hasGLExtension(ext) glewIsSupported("GL_" #ext)
#endif
#ifdef USE_GLAD
#define hasGLExtension(ext) GLAD_GL_## ext
#endif

std::string gl_dump();
std::string gl_extensions_dump();
