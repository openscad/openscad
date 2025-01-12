#include "glview/VertexState.h"

void VertexState::draw(bool bind_buffers) const
{
  if (vertices_vbo_ && bind_buffers) {
    GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertices_vbo_);
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_));
  }
  if (elements_vbo_ && bind_buffers) {
    GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", elements_vbo_);
    GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_));
  }
  for (const auto& gl_func : gl_begin_) {
    gl_func();
  }
  if (draw_size_ > 0) {
    if (elements_vbo_) {
      GL_TRACE("glDrawElements(%s, %d, %s, %d)",
               (draw_mode_ == GL_POINTS ? "GL_POINTS" :
                draw_mode_ == GL_LINES ? "GL_LINES" :
                draw_mode_ == GL_LINE_LOOP ? "GL_LINE_LOOP" :
                draw_mode_ == GL_LINE_STRIP ? "GL_LINE_STRIP" :
                draw_mode_ == GL_TRIANGLES ? "GL_TRIANGLES" :
                draw_mode_ == GL_TRIANGLE_STRIP ? "GL_TRIANGLE_STRIP" :
                draw_mode_ == GL_TRIANGLE_FAN ? "GL_TRIANGLE_FAN" :
                draw_mode_ == GL_QUADS ? "GL_QUADS" :
                draw_mode_ == GL_QUAD_STRIP ? "GL_QUAD_STRIP" :
                draw_mode_ == GL_POLYGON ? "GL_POLYGON" :
                "UNKNOWN") % draw_size_ %
               (draw_type_ == GL_UNSIGNED_BYTE ? "GL_UNSIGNED_BYTE" :
                draw_type_ == GL_UNSIGNED_SHORT ? "GL_UNSIGNED_SHORT" :
                draw_type_ == GL_UNSIGNED_INT ? "GL_UNSIGNED_INT" :
                "UNKNOWN") % element_offset_);
      // NOLINTNEXTLINE(performance-no-int-to-ptr)
      glDrawElements(draw_mode_, draw_size_, draw_type_, (GLvoid *)element_offset_);
    } else {
      GL_TRACE("glDrawArrays(%s, 0, %d)",
               (draw_mode_ == GL_POINTS ? "GL_POINTS" :
                draw_mode_ == GL_LINES ? "GL_LINES" :
                draw_mode_ == GL_LINE_LOOP ? "GL_LINE_LOOP" :
                draw_mode_ == GL_LINE_STRIP ? "GL_LINE_STRIP" :
                draw_mode_ == GL_TRIANGLES ? "GL_TRIANGLES" :
                draw_mode_ == GL_TRIANGLE_STRIP ? "GL_TRIANGLE_STRIP" :
                draw_mode_ == GL_TRIANGLE_FAN ? "GL_TRIANGLE_FAN" :
                draw_mode_ == GL_QUADS ? "GL_QUADS" :
                draw_mode_ == GL_QUAD_STRIP ? "GL_QUAD_STRIP" :
                draw_mode_ == GL_POLYGON ? "GL_POLYGON" :
                "UNKNOWN") % draw_size_);
      glDrawArrays(draw_mode_, 0, draw_size_);
    }
  }
  for (const auto& gl_func : gl_end_) {
    gl_func();
  }
  if (elements_vbo_ && bind_buffers) {
    GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  }
  if (vertices_vbo_ && bind_buffers) {
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }
}
