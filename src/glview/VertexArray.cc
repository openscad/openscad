#include "glview/VertexArray.h"

#include <cstring>
#include <cassert>
#include <array>
#include <utility>
#include <vector>
#include <memory>
#include <cstdio>

#include "utils/printutils.h"

void addAttributeValues(IAttributeData&) {}

void VertexData::getLastVertex(std::vector<GLbyte>& interleaved_buffer) const
{
  GLbyte *dst_start = interleaved_buffer.data();
  for (const auto& data : attributes_) {
    size_t size = data->sizeofAttribute();
    GLbyte *dst = dst_start;
    const GLbyte *src = data->toBytes() + data->sizeInBytes() - data->sizeofAttribute();
    std::memcpy((void *)dst, (void *)src, size);
    dst_start += size;
  }
}

void VertexData::remove(size_t count)
{
  for (const auto& data : attributes_) {
    data->remove(count);
  }
}

void VertexArray::addSurfaceData()
{
  std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
  vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat, 3, GL_FLOAT>>());
  vertex_data->addNormalData(std::make_shared<AttributeData<GLfloat, 3, GL_FLOAT>>());
  vertex_data->addColorData(std::make_shared<AttributeData<GLfloat, 4, GL_FLOAT>>());
  surface_index_ = vertices_.size();
  addVertexData(vertex_data);
}

void VertexArray::addEdgeData()
{
  std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
  vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat, 3, GL_FLOAT>>());
  vertex_data->addColorData(std::make_shared<AttributeData<GLfloat, 4, GL_FLOAT>>());
  edge_index_ = vertices_.size();
  addVertexData(vertex_data);
}

void VertexArray::createVertex(const std::array<Vector3d, 3>& points,
                               const std::array<Vector3d, 3>& normals,
                               const Color4f& color,
                               size_t active_point_index, size_t primitive_index,
                               size_t shape_size, bool outlines, bool mirror,
                               const CreateVertexCallback& vertex_callback)
{
  if (vertex_callback) {
    vertex_callback(*this, active_point_index,
                    primitive_index, shape_size, outlines);
  }
  
  addAttributeValues(*(data()->positionData()), points[active_point_index][0], points[active_point_index][1], points[active_point_index][2]);
  if (data()->hasNormalData()) {
    addAttributeValues(*(data()->normalData()), normals[active_point_index][0], normals[active_point_index][1], normals[active_point_index][2]);
  }
  if (data()->hasColorData()) {
    addAttributeValues(*(data()->colorData()), color[0], color[1], color[2], color[3]);
  }

  if (useElements()) {
    std::vector<GLbyte> interleaved_vertex;
    interleaved_vertex.resize(data()->stride());
    data()->getLastVertex(interleaved_vertex);
    std::pair<ElementsMap::iterator, bool> entry;
    entry.first = elements_map_.find(interleaved_vertex);
    if (entry.first == elements_map_.end()) {
      // append vertex data if this is a new element
      if (vertices_size_) {
        if (interleaved_buffer_.size()) {
          memcpy(interleaved_buffer_.data() + vertices_offset_, interleaved_vertex.data(), interleaved_vertex.size());
        } else {
          GL_TRACE("glBufferSubData(GL_ARRAY_BUFFER, %d, %d, %p)", vertices_offset_ % interleaved_vertex.size() % interleaved_vertex.data());
          GL_CHECKD(glBufferSubData(GL_ARRAY_BUFFER, vertices_offset_, interleaved_vertex.size(), interleaved_vertex.data()));
        }
        data()->clear();
      }
      vertices_offset_ += interleaved_vertex.size();
      entry = elements_map_.emplace(interleaved_vertex, elements_map_.size());
    } else {
      data()->remove();
#if 0
      if (OpenSCAD::debug != "") {
        // in debug, check for bad hash matches
        size_t i = 0;
        if (interleaved_vertex.size() != entry.first->first.size()) {
          PRINTDB("vertex index = %d", entry.first->second);
          assert(false && "VBORenderer invalid vertex match size!!!");
        }
        for (const auto& b : interleaved_vertex) {
          if (b != entry.first->first[i]) {
            PRINTDB("vertex index = %d", entry.first->second);
            assert(false && "VBORenderer invalid vertex value hash match!!!");
          }
          i++;
        }
      }
#endif // 0
    }

    // append element data
    addAttributeValues(*elementsData(), entry.first->second);
    elements_offset_ += elementsData()->sizeofAttribute();
  } else { // !useElements()
    if (!vertices_size_) {
      vertices_offset_ = sizeInBytes();
    } else {
      std::vector<GLbyte> interleaved_vertex;
      interleaved_vertex.resize(data()->stride());
      data()->getLastVertex(interleaved_vertex);
      if (interleaved_buffer_.size()) {
        memcpy(interleaved_buffer_.data() + vertices_offset_, interleaved_vertex.data(), interleaved_vertex.size());
      } else {
	// This path is chosen in vertex-object-renderers-direct mode
        GL_TRACE("B glBufferSubData(GL_ARRAY_BUFFER, %d, %d, %p)", vertices_offset_ % interleaved_vertex.size() % interleaved_vertex.data());
        GL_CHECKD(glBufferSubData(GL_ARRAY_BUFFER, vertices_offset_, interleaved_vertex.size(), interleaved_vertex.data()));
      }
      vertices_offset_ += interleaved_vertex.size();
      data()->clear();
    }
  }
}

void VertexArray::createInterleavedVBOs()
{
  for (const auto& state : states_) {
    size_t index = state->drawOffset();
    state->drawOffset(this->indexOffset(index));
  }

  // If the upfront size was not known, the the buffer has to be built
  size_t total_size = this->sizeInBytes();
  // If VertexArray is not empty, and initial size is zero
  if (!vertices_size_ && total_size) {
    GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_));
    GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", total_size % (void *)nullptr);
    GL_CHECKD(glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_STATIC_DRAW));

    size_t dst_start = 0;
    for (const auto& vertex_data : vertices_) {
      // All attribute vectors need to be the same size to interleave
      size_t idx = 0, last_size = 0, stride = vertex_data->stride();
      for (const auto& data : vertex_data->attributes()) {
        size_t size = data->sizeofAttribute();
        const GLbyte *src = data->toBytes();
        size_t dst = dst_start;

        if (src) {
          if (idx != 0) {
            if (last_size != data->size() / data->count()) {
              PRINTDB("attribute data for vertex incorrect size at index %d = %d", idx % (data->size() / data->count()));
              PRINTDB("last_size = %d", last_size);
              assert(false);
            }
          }
          last_size = data->size() / data->count();
          for (size_t i = 0; i < last_size; ++i) {
	    // This path is chosen in vertex-object-renderers non-direct mode
            GL_TRACE("A glBufferSubData(GL_ARRAY_BUFFER, %p, %d, %p)", (void *)dst % size % (void *)src);
            GL_CHECKD(glBufferSubData(GL_ARRAY_BUFFER, dst, size, src));
            src += size;
            dst += stride;
          }
          dst_start += size;
        }
        idx++;
      }
      dst_start = vertex_data->sizeInBytes();
    }

    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));
  } else if (vertices_size_ && interleaved_buffer_.size()) {
    GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_));
    GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", interleaved_buffer_.size() % (void *)interleaved_buffer_.data());
    GL_CHECKD(glBufferData(GL_ARRAY_BUFFER, interleaved_buffer_.size(), interleaved_buffer_.data(), GL_STATIC_DRAW));
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }

  PRINTDB("useElements() = %d, elements_size_ = %d", useElements() % elements_size_);
  if (useElements()) {
    GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", elements_vbo_);
    GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_));
    if (elements_size_ == 0) {
      GL_TRACE("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_.sizeInBytes() % (void *)nullptr);
      GL_CHECKD(glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_.sizeInBytes(), nullptr, GL_STATIC_DRAW));
    }
    size_t last_size = 0;
    for (const auto& e : elements_.attributes()) {
      GL_TRACE("glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, %d, %d, %p)", last_size % e->sizeInBytes() % (void *)e->toBytes());
      GL_CHECKD(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, last_size, e->sizeInBytes(), e->toBytes()));
      last_size += e->sizeInBytes();
    }
    GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  }
}

void VertexArray::addAttributePointers(size_t start_offset)
{
  if (!this->data()) return;

  std::shared_ptr<VertexData> vertex_data = this->data();
  std::shared_ptr<VertexState> vs = states_.back();

  GLsizei count = vertex_data->positionData()->count();
  GLenum type = vertex_data->positionData()->glType();
  GLsizei stride = vertex_data->stride();
  size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->positionIndex());
  // Note: Some code, like OpenCSGRenderer::createVBOPrimitive() relies on this order of
  // glBegin/glEnd functions for unlit/uncolored vertex rendering.
  vs->glBegin().emplace_back([]() {
    GL_TRACE0("glEnableClientState(GL_VERTEX_ARRAY)");
    GL_CHECKD(glEnableClientState(GL_VERTEX_ARRAY));
  });
  vs->glBegin().emplace_back([count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
    auto vs = vs_ptr.lock();
    if (vs) {
      // NOLINTBEGIN(performance-no-int-to-ptr)
      GL_TRACE("glVertexPointer(%d, %d, %d, %p)",
               count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
      GL_CHECKD(glVertexPointer(count, type, stride, (GLvoid *)(vs->drawOffset() + offset)));
      // NOLINTEND(performance-no-int-to-ptr)
    }
  });
  vs->glEnd().emplace_back([]() {
    GL_TRACE0("glDisableClientState(GL_VERTEX_ARRAY)");
    GL_CHECKD(glDisableClientState(GL_VERTEX_ARRAY));
  });

  if (vertex_data->hasNormalData()) {
    type = vertex_data->normalData()->glType();
    size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->normalIndex());
    vs->glBegin().emplace_back([]() {
      GL_TRACE0("glEnableClientState(GL_NORMAL_ARRAY)");
      GL_CHECKD(glEnableClientState(GL_NORMAL_ARRAY));
    });
    vs->glBegin().emplace_back([type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
      auto vs = vs_ptr.lock();
      if (vs) {
        // NOLINTBEGIN(performance-no-int-to-ptr)
        GL_TRACE("glNormalPointer(%d, %d, %p)", type % stride % (GLvoid *)(vs->drawOffset() + offset));
        GL_CHECKD(glNormalPointer(type, stride, (GLvoid *)(vs->drawOffset() + offset)));
        // NOLINTEND(performance-no-int-to-ptr)
      }
    });
    vs->glEnd().emplace_back([]() {
      GL_TRACE0("glDisableClientState(GL_NORMAL_ARRAY)");
      GL_CHECKD(glDisableClientState(GL_NORMAL_ARRAY));
    });
  }
  if (vertex_data->hasColorData()) {
    count = vertex_data->colorData()->count();
    type = vertex_data->colorData()->glType();
    size_t offset = start_offset + vertex_data->interleavedOffset(vertex_data->colorIndex());
    vs->glBegin().emplace_back([]() {
      GL_TRACE0("glEnableClientState(GL_COLOR_ARRAY)");
      GL_CHECKD(glEnableClientState(GL_COLOR_ARRAY));
    });
    vs->glBegin().emplace_back([count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
      auto vs = vs_ptr.lock();
      if (vs) {
        // NOLINTBEGIN(performance-no-int-to-ptr)
        GL_TRACE("glColorPointer(%d, %d, %d, %p)", count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
        GL_CHECKD(glColorPointer(count, type, stride, (GLvoid *)(vs->drawOffset() + offset)));
        // NOLINTEND(performance-no-int-to-ptr)
      }
    });
    vs->glEnd().emplace_back([]() {
      GL_TRACE0("glDisableClientState(GL_COLOR_ARRAY)");
      GL_CHECKD(glDisableClientState(GL_COLOR_ARRAY));
    });
  }
}

// Allocates GPU memory for vertices (and elements if enabled)
// for holding the given number of vertices.
void VertexArray::allocateBuffers(size_t num_vertices) {
  size_t vertices_size = num_vertices * stride();
  setVerticesSize(vertices_size);
  GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
  GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_));
  GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", vertices_size % (void *)nullptr);
  GL_CHECKD(glBufferData(GL_ARRAY_BUFFER, vertices_size, nullptr, GL_STATIC_DRAW));
  if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
    // Use smallest possible index data type
    if (num_vertices <= 0xff) {
      addElementsData(std::make_shared<AttributeData<GLubyte, 1, GL_UNSIGNED_BYTE>>());
    } else if (num_vertices <= 0xffff) {
      addElementsData(std::make_shared<AttributeData<GLushort, 1, GL_UNSIGNED_SHORT>>());
    } else {
      addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
    }
    // FIXME: How do we know how much to allocate?
    size_t elements_size = num_vertices * elements_.stride();
    setElementsSize(elements_size);
    GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", elements_vbo_);
    GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_));
    GL_TRACE("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_size % (void *)nullptr);
    GL_CHECKD(glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_size, nullptr, GL_STATIC_DRAW));
  }
}
