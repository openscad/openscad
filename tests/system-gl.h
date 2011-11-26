#ifndef SYSTEMGL_H_
#define SYSTEMGL_H_

#include <GL/glew.h>

void glew_dump(bool dumpall = false);
bool report_glerror(const char *task);

#endif
