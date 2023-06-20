// The name of this file is a placeholder, used for collecting commonly used operations in code deduplication
#pragma once

#include "Renderer.h"
#include "system-gl.h"

#include "VBORenderer.h"

class VertexStateManager {
public:
  VertexStateManager(VertexStates* v_s, VertexArray* v_a) : vertex_states(v_s), vertex_array(v_a), placeholder() {}
  void initializeSize(size_t vertices_size, bool multiple_vbo = false, std::vector<GLuint>::iterator current_vbo = placeholder.begin());
  void addColorState(Color4f color);
private:
  VertexStates* vertex_states;
  VertexArray* vertex_array;
  std::vector<GLuint> placeholder;
};