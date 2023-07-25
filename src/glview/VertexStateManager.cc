#include "VertexStateManager.h"

void VertexStateManager::initializeSize(size_t vertices_size) {
  std::vector<GLuint> placeholder;
  size_t placeholder_index = 0;
  initializeSizeHelper(vertices_size, false, placeholder, placeholder_index);
}

void VertexStateManager::initializeSize(size_t vertices_size, std::vector<GLuint> & vbos, size_t & vbo_index) {
  initializeSizeHelper(vertices_size, true, vbos, vbo_index);
}

void VertexStateManager::initializeSizeHelper(size_t vertices_size, bool multiple_vbo, std::vector<GLuint> & vbos, size_t & vbo_index) {
  if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
    size_t elements_size = 0;
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      if (multiple_vbo) {
        vertex_array.elementsVBO() = vbos[vbo_index++];
      }
      if (vertices_size <= 0xff) {
          vertex_array.addElementsData(std::make_shared<AttributeData<GLubyte, 1, GL_UNSIGNED_BYTE>>());
      } else if (vertices_size <= 0xffff) {
          vertex_array.addElementsData(std::make_shared<AttributeData<GLushort, 1, GL_UNSIGNED_SHORT>>());
      } else {
          vertex_array.addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
      }
      elements_size = vertices_size * vertex_array.elements().stride();
      vertex_array.elementsSize(elements_size);
    }
    vertices_size *= vertex_array.stride();
    vertex_array.verticesSize(vertices_size);

    GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertex_array.verticesVBO());
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, vertex_array.verticesVBO()));
    GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", vertices_size % (void *)nullptr);
    GL_CHECKD(glBufferData(GL_ARRAY_BUFFER, vertices_size, nullptr, GL_STATIC_DRAW));
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", vertex_array.elementsVBO());
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_array.elementsVBO()));
      GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_size % (void *)nullptr);
      GL_CHECKD(glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_size, nullptr, GL_STATIC_DRAW));
    }
  } else if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
    if (multiple_vbo) {
      vertex_array.elementsVBO() = vbos[vbo_index++];
    }
    vertex_array.addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
  }
}
void VertexStateManager::addColor(const Color4f& last_color) {
  renderer.add_shader_pointers(vertex_array);
  Renderer::shaderinfo_t shader_info = renderer.getShader();
  std::shared_ptr<VertexState> color_state = std::make_shared<VBOShaderVertexState>(0, 0, vertex_array.verticesVBO(), vertex_array.elementsVBO());
  color_state->glBegin().emplace_back([shader_info, last_color]() {
    GL_TRACE("glUniform4f(%d, %f, %f, %f, %f)", shader_info.data.csg_rendering.color_area % last_color[0] % last_color[1] % last_color[2] % last_color[3]);
    GL_CHECKD(glUniform4f(shader_info.data.csg_rendering.color_area, last_color[0], last_color[1], last_color[2], last_color[3]));
    GL_TRACE("glUniform4f(%d, %f, %f, %f, 1.0)", shader_info.data.csg_rendering.color_edge % ((last_color[0] + 1) / 2) % ((last_color[1] + 1) / 2) % ((last_color[2] + 1) / 2));
    GL_CHECKD(glUniform4f(shader_info.data.csg_rendering.color_edge, (last_color[0] + 1) / 2, (last_color[1] + 1) / 2, (last_color[2] + 1) / 2, 1.0));
  });
  vertex_array.states().emplace_back(std::move(color_state));
}