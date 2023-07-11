#pragma once
// The name of this file is a placeholder, used for collecting commonly used operations in code deduplication

#include "Renderer.h"
#include "system-gl.h"

#include "VBORenderer.h"

class VertexStateManager {
public:
  VertexStateManager(VBORenderer& r, VertexArray& v_a) : vertex_array(v_a), renderer(r) {}
  void initializeSize(size_t vertices_size);
  void initializeSize(size_t vertices_size, std::vector<GLuint> & vbos, size_t & vbo_index);
  void addColor(const Color4f& last_color);
private:
  VBORenderer& renderer;
  VertexArray& vertex_array;
  void initializeSizeHelper(size_t vertices_size, bool multiple_vbo, std::vector<GLuint> & vbos, size_t & vbo_index);
};