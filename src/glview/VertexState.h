#pragma once

#include <memory>
#include <cstddef>
#include <functional>
#include <vector>

#include "glview/system-gl.h"

#define GL_TRACE_ENABLE
#ifdef GL_TRACE_ENABLE
// NOLINTBEGIN(bugprone-macro-parentheses)
#define GL_TRACE(fmt_, args) do { \
          if (OpenSCAD::debug != "") PRINTDB("%d : " fmt_, __LINE__ % args); \
} while (0)
// NOLINTEND(bugprone-macro-parentheses)
#define GL_TRACE0(fmt_) do { \
          if (OpenSCAD::debug != "") PRINTDB("%d : " fmt_, __LINE__); \
} while (0)
#else // GL_TRACE_ENABLE
#define GL_TRACE(fmt_, args) do {} while (0)
#define GL_TRACE0(fmt_) do {} while (0)
#endif // GL_TRACE_ENABLE

// Storage for minimum state information necessary to draw VBO.
// This class is also used to encapsulate other state;
// Example: VBOShaderVertexState will not render anything, but will use glBegin() to e.g. manage shader uniforms.
class VertexState
{
public:
  VertexState()
    : draw_mode_(GL_TRIANGLES), draw_size_(0), draw_type_(0), draw_offset_(0),
    element_offset_(0), vertices_vbo_(0), elements_vbo_(0)
  {}
  VertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type, size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo)
    : draw_mode_(draw_mode), draw_size_(draw_size), draw_type_(draw_type), draw_offset_(draw_offset),
    element_offset_(element_offset), vertices_vbo_(vertices_vbo), elements_vbo_(elements_vbo)
  {}
  virtual ~VertexState() = default;

  // Return the OpenGL mode for glDrawArrays/glDrawElements call
  [[nodiscard]] inline GLenum drawMode() const { return draw_mode_; }
  // Set the OpenGL mode for glDrawArrays/glDrawElements call
  inline void drawMode(GLenum draw_mode) { draw_mode_ = draw_mode; }
  // Return the number of vertices for glDrawArrays/glDrawElements call
  [[nodiscard]] inline GLsizei drawSize() const { return draw_size_; }
  // Set the number of vertices for glDrawArrays/glDrawElements call
  inline void drawSize(GLsizei draw_size) { draw_size_ = draw_size; }
  // Return the OpenGL type for glDrawElements call
  [[nodiscard]] inline GLenum drawType() const { return draw_type_; }
  // Set the OpenGL type for glDrawElements call
  inline void drawType(GLenum draw_type) { draw_type_ = draw_type; }
  // Return the VBO offset for glDrawArrays call
  [[nodiscard]] inline size_t drawOffset() const { return draw_offset_; }
  // Set the VBO offset for glDrawArrays call
  inline void drawOffset(size_t draw_offset) { draw_offset_ = draw_offset; }
  // Return the Element VBO offset for glDrawElements call
  [[nodiscard]] inline size_t elementOffset() const { return element_offset_; }
  // Set the Element VBO offset for glDrawElements call
  inline void elementOffset(size_t element_offset) { element_offset_ = element_offset; }

  // Wrap glDrawArrays/glDrawElements call and use gl_begin/gl_end state information
  virtual void draw(bool bind_buffers = true) const;

  // Mimic VAO state functionality. Lambda functions used to hold OpenGL state calls.
  inline std::vector<std::function<void()>>& glBegin() { return gl_begin_; }
  inline std::vector<std::function<void()>>& glEnd() { return gl_end_; }

  [[nodiscard]] inline GLuint verticesVBO() const { return vertices_vbo_; }
  inline void verticesVBO(GLuint vbo) { vertices_vbo_ = vbo; }
  [[nodiscard]] inline GLuint elementsVBO() const { return elements_vbo_; }
  inline void elementsVBO(GLuint vbo) { elements_vbo_ = vbo; }

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
  [[nodiscard]] virtual std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size, GLenum draw_type, size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo) const {
    return std::make_shared<VertexState>(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo);
  }
};
