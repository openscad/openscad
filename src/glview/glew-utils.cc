#include "glview/glew-utils.h"
#include <cstdio>

#include <GL/glew.h>

bool initializeGlew() {
  auto err = glewInit();
  if (err != GLEW_OK) {
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
    return false;
  }
  return true;
}
