#pragma once

#include <utility>
#include <memory>
#include <cstddef>
#include <functional>
#include <vector>

#include "glview/system-gl.h"
#include "Feature.h"

#define GL_TRACE_ENABLE
#ifdef GL_TRACE_ENABLE
// NOLINTBEGIN(bugprone-macro-parentheses)
#define GL_TRACE(fmt_, args)                                           \
  do {                                                                 \
    if (OpenSCAD::debug != "") PRINTDB("%d : " fmt_, __LINE__ % args); \
  } while (0)
// NOLINTEND(bugprone-macro-parentheses)
#define GL_TRACE0(fmt_)                                         \
  do {                                                          \
    if (OpenSCAD::debug != "") PRINTDB("%d : " fmt_, __LINE__); \
  } while (0)
#else  // GL_TRACE_ENABLE
#define GL_TRACE(fmt_, args) \
  do {                       \
  } while (0)
#define GL_TRACE0(fmt_) \
  do {                  \
  } while (0)
#endif  // GL_TRACE_ENABLE

// Storage for minimum state information necessary to draw VBO.
// This class is also used to encapsulate other state;
// Example: VBOShaderVertexState will not render anything, but will use glBegin() to e.g. manage shader
// uniforms.
class VertexState
{
public:
  VertexState()
    : draw_mode_(GL_TRIANGLES),
      draw_size_(0),
      draw_type_(0),
      draw_offset_(0),
      element_offset_(0),
      vertices_vbo_(0),
      elements_vbo_(0)
  {
  }
  VertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type, size_t draw_offset,
              size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo)
    : draw_mode_(draw_mode),
      draw_size_(draw_size),
      draw_type_(draw_type),
      draw_offset_(draw_offset),
      element_offset_(element_offset),
      vertices_vbo_(vertices_vbo),
      elements_vbo_(elements_vbo)
  {
  }
  virtual ~VertexState() = default;

  // Return the OpenGL mode for glDrawArrays/glDrawElements call
  [[nodiscard]] inline GLenum drawMode() const { return draw_mode_; }
  // Set the OpenGL mode for glDrawArrays/glDrawElements call
  inline void setDrawMode(GLenum draw_mode) { draw_mode_ = draw_mode; }
  // Return the number of vertices for glDrawArrays/glDrawElements call
  [[nodiscard]] inline GLsizei drawSize() const { return draw_size_; }
  // Set the number of vertices for glDrawArrays/glDrawElements call
  inline void setDrawSize(GLsizei draw_size) { draw_size_ = draw_size; }
  // Return the OpenGL type for glDrawElements call
  [[nodiscard]] inline GLenum drawType() const { return draw_type_; }
  // Set the OpenGL type for glDrawElements call
  inline void setDrawType(GLenum draw_type) { draw_type_ = draw_type; }
  // Return the VBO offset for glDrawArrays call
  [[nodiscard]] inline size_t drawOffset() const { return draw_offset_; }
  // Set the VBO offset for glDrawArrays call
  inline void setDrawOffset(size_t draw_offset) { draw_offset_ = draw_offset; }
  // Return the Element VBO offset for glDrawElements call
  [[nodiscard]] inline size_t elementOffset() const { return element_offset_; }
  // Set the Element VBO offset for glDrawElements call
  inline void setElementOffset(size_t element_offset) { element_offset_ = element_offset; }

  // Wrap glDrawArrays/glDrawElements call and use gl_begin/gl_end state information
  virtual void draw() const;

  // Mimic VAO state functionality. Lambda functions used to hold OpenGL state calls.
  inline std::vector<std::function<void()>>& glBegin() { return gl_begin_; }
  inline std::vector<std::function<void()>>& glEnd() { return gl_end_; }

  [[nodiscard]] inline GLuint verticesVBO() const { return vertices_vbo_; }
  inline void setVerticesVBO(GLuint vbo) { vertices_vbo_ = vbo; }
  [[nodiscard]] inline GLuint elementsVBO() const { return elements_vbo_; }
  inline void setElementsVBO(GLuint vbo) { elements_vbo_ = vbo; }

private:
  GLenum draw_mode_;
  GLsizei draw_size_;
  GLenum draw_type_;
  size_t draw_offset_;
  size_t element_offset_;
  GLuint vertices_vbo_;
  GLuint elements_vbo_;
  std::vector<std::function<void()>> gl_begin_;
  std::vector<std::function<void()>> gl_end_;
};

// Allows Renderers to override VertexState objects with their own derived
// type. VertexArray will create the appropriate type for creating
// a VertexState object.
class VertexStateFactory
{
public:
  VertexStateFactory() = default;
  virtual ~VertexStateFactory() = default;

  // Create and return a VertexState object
  [[nodiscard]] virtual std::shared_ptr<VertexState> createVertexState(
    GLenum draw_mode, size_t draw_size, GLenum draw_type, size_t draw_offset, size_t element_offset,
    GLuint vertices_vbo, GLuint elements_vbo) const
  {
    return std::make_shared<VertexState>(draw_mode, draw_size, draw_type, draw_offset, element_offset,
                                         vertices_vbo, elements_vbo);
  }
};

class VertexStateContainer
{
public:
  VertexStateContainer()
  {
    GL_TRACE("glGenBuffers(1, %p)", &vertices_vbo_);
    GL_CHECKD(glGenBuffers(1, &vertices_vbo_));
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE("glGenBuffers(1, %p)", &elements_vbo_);
      GL_CHECKD(glGenBuffers(1, &elements_vbo_));
    }
  }
  VertexStateContainer(const VertexStateContainer& o) = delete;
  VertexStateContainer(VertexStateContainer&& o) noexcept
  {
    vertices_vbo_ = o.vertices_vbo_;
    elements_vbo_ = o.elements_vbo_;
    vertex_states_ = std::move(o.vertex_states_);
    o.vertices_vbo_ = 0;
    o.elements_vbo_ = 0;
  }

  virtual ~VertexStateContainer()
  {
    if (vertices_vbo_) {
      GL_TRACE("glDeleteBuffers(1, %p)", &vertices_vbo_);
      GL_CHECKD(glDeleteBuffers(1, &vertices_vbo_));
    }
    if (elements_vbo_) {
      GL_TRACE("glDeleteBuffers(1, %p)", &elements_vbo_);
      GL_CHECKD(glDeleteBuffers(1, &elements_vbo_));
    }
  }

  GLuint verticesVBO() const { return vertices_vbo_; }
  GLuint elementsVBO() const { return elements_vbo_; }

  std::vector<std::shared_ptr<VertexState>>& states() { return vertex_states_; }
  const std::vector<std::shared_ptr<VertexState>>& states() const { return vertex_states_; }

private:
  GLuint vertices_vbo_;
  GLuint elements_vbo_ = 0;

  std::vector<std::shared_ptr<VertexState>> vertex_states_;
};
