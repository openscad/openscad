#pragma once
// The name of this file is a placeholder, used for collecting commonly used operations in code deduplication

#include "Renderer.h"
#include "system-gl.h"

#include "VBORenderer.h"

class VertexStateManager {
public:
  VertexStateManager(VertexStates* v_s, VertexArray* v_a) : vertex_states(v_s), vertex_array(v_a) {}
  void initializeSize(size_t vertices_size);
  void initializeSize(size_t vertices_size, std::vector<GLuint> & vbos, size_t & vbo_index);
  void addColorState(Color4f color);
private:
  VertexStates* vertex_states;
  VertexArray* vertex_array;
  void initializeSizeHelper(size_t vertices_size, bool multiple_vbo, std::vector<GLuint> & vbos, size_t & vbo_index);
};