#include "glew-utils.h"

#include <cstdio>
#include <sstream>

#include <GL/glew.h>

bool initializeGlew() {
  auto err = glewInit();
  if (err != GLEW_OK) {
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
    return false;
  }
  return true;
}

std::string glewInfo() {
  std::ostringstream out;
  out << "GLEW version: " << glewGetString(GLEW_VERSION);
  return out.str();
}
