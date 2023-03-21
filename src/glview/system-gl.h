#pragma once

#ifndef NULLGL
#include <GL/glew.h>

#ifdef __APPLE__
 #include <OpenGL/OpenGL.h>
#else
 #include <GL/gl.h>
 #include <GL/glu.h>
#endif

#include <string>
#include "printutils.h"

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

// We have 3 different GL checks
// GL_DEBUG_CHECKD(statement, [onerror])
//   Outputs PRINTDB() in debug mode only. Executes onerror statement on error, if given.
//   This is fast in release mode (no-op if the onerror statement isn't provided).
// GL_CHECK(statement, onerror) Outputs LOG(Error) in both release and debug mode. Executes onerror statement on error
//   Always prints the output on error.
// GL_CHECKD(statement, [onerror]) Outputs PRINTDB() in both release and debug mode. Executes onerror statement on error, if given
//   Prints the output on error only if --debug is used

// GL_DEBUG_CHECKD(statement, [onerror])
// Use this for OpenGL debug output which needs to be fast in release mode
// Debug output is only available for debug build and only when using --enable= at runtime
// This example will print an error if glClear() fails if --debug is specified, but yields just glClear in release mode.
// GL_DEBUG_CHECKD(glClear());
#ifdef DEBUG
#define GET_GL_DEBUG_CHECKD(_1, _2, NAME, ...) NAME
#define GL_DEBUG_CHECKD_ONLY(stmt) stmt; glCheckd(#stmt, __FILE__, __LINE__)
#define GL_DEBUG_CHECKD_ERR(stmt, onerror) stmt; if (!glCheckd(#stmt, __FILE__, __LINE__)) { onerror; }
#define GL_DEBUG_CHECKD(...) GET_GL_CHECKD(__VA_ARGS__, GL_DEBUG_CHECKD_ERR, GL_DEBUG_CHECKD_ONLY)(__VA_ARGS__)
#else
#define GET_GL_DEBUG_CHECKD(_1, _2, NAME, ...) NAME
#define GL_DEBUG_CHECKD_ONLY(stmt) stmt
#define GL_DEBUG_CHECKD_ERR(stmt, onerror) stmt; if (glGetError != GL_NO_ERROR) { onerror; }
#define GL_DEBUG_CHECKD(...) GET_GL_CHECKD(__VA_ARGS__, GL_DEBUG_CHECKD_ERR, GL_DEBUG_CHECKD_ONLY)(__VA_ARGS__)
#endif

// GL_CHECK(statement, onerror)
// Use this for important error handling which always causes an error, also in release mode
// This example will print an error and return false if glClear() fails:
// GL_CHECK(glClear(), return false);
#define GL_CHECK(stmt, onerror) stmt; if (!glCheck(#stmt, __FILE__, __LINE__)) { onerror; }

// GL_CHECKD(statement, [onerror])
// Use this for OpenGL debug error output which should make it into the release build
// Enable debug output at runtime using --enable=
// This example will print an error if --debug is specified and return if glClear() fails:
// GL_CHECK(glClear(), return);
#define GET_GL_CHECKD(_1, _2, NAME, ...) NAME
#define GL_CHECKD_ONLY(stmt) stmt; glCheckd(#stmt, __FILE__, __LINE__)
#define GL_CHECKD_ERR(stmt, onerror) stmt; if (!glCheckd(#stmt, __FILE__, __LINE__)) { onerror; }
#define GL_CHECKD(...) GET_GL_CHECKD(__VA_ARGS__, GL_CHECKD_ERR, GL_CHECKD_ONLY)(__VA_ARGS__)

#else // NULLGL

#define GLint int
#define GLuint unsigned int
inline void glColor4fv(float *c) {}

#endif // NULLGL

std::string glew_dump();
std::string glew_extensions_dump();
