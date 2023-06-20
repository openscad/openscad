#include "VertexStateManager.h"

void VertexStateManager::initializeSize(size_t vertices_size) {
  std::vector<GLuint> placeholder = std::vector<GLuint>();
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
        vertex_array->elementsVBO() = vbos[vbo_index++];
      }
      if (vertices_size <= 0xff) {
          vertex_array->addElementsData(std::make_shared<AttributeData<GLubyte, 1, GL_UNSIGNED_BYTE>>());
      } else if (vertices_size <= 0xffff) {
          vertex_array->addElementsData(std::make_shared<AttributeData<GLushort, 1, GL_UNSIGNED_SHORT>>());
      } else {
          vertex_array->addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
      }
      elements_size = vertices_size * vertex_array->elements().stride();
      vertex_array->elementsSize(elements_size);
    }
    vertices_size *= vertex_array->stride();
    vertex_array->verticesSize(vertices_size);

    GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertex_array->verticesVBO());
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, vertex_array->verticesVBO()));
    GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", vertices_size % (void *)nullptr);
    GL_CHECKD(glBufferData(GL_ARRAY_BUFFER, vertices_size, nullptr, GL_STATIC_DRAW));
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", vertex_array->elementsVBO());
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_array->elementsVBO()));
      GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_size % (void *)nullptr);
      GL_CHECKD(glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_size, nullptr, GL_STATIC_DRAW));
    }
  } else if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
    if (multiple_vbo) {
      vertex_array->elementsVBO() = vbos[vbo_index++];
    }
    vertex_array->addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
  }
}