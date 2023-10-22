#pragma once
// The name of this file is a placeholder, used for collecting commonly used operations in code deduplication

#include "Renderer.h"
#include "system-gl.h"

#include "VBORenderer.h"

class VertexStateManager {
public:
  VertexStateManager(VBORenderer& r, VertexArray& v_a) : vertex_array(v_a), renderer(r) {}
  void initializeSize(size_t num_vertices);
  void addColor(const Color4f& color);
private:
  VBORenderer& renderer;
  VertexArray& vertex_array;
};
