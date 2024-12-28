/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "glview/VBORenderer.h"
#include "geometry/PolySet.h"
#include "core/CSGNode.h"
#include "utils/printutils.h"
#include "utils/hash.h" // IWYU pragma: keep

#include <cassert>
#include <array>
#include <unordered_map>
#include <utility>
#include <memory>
#include <cstddef>

VBORenderer::VBORenderer()
  : Renderer()
{
}

void VBORenderer::resize(int w, int h)
{
  Renderer::resize(w, h);
}

bool VBORenderer::getShaderColor(Renderer::ColorMode colormode, const Color4f& color, Color4f& outcolor) const
{
  if ((colormode != ColorMode::BACKGROUND) && (colormode != ColorMode::HIGHLIGHT) && color.isValid()) {
    outcolor = color;
    return true;
  }
  Color4f basecol;
  if (Renderer::getColor(colormode, basecol)) {
    if (colormode == ColorMode::BACKGROUND || colormode != ColorMode::HIGHLIGHT) {
      basecol = Color4f(color[0] >= 0 ? color[0] : basecol[0],
                        color[1] >= 0 ? color[1] : basecol[1],
                        color[2] >= 0 ? color[2] : basecol[2],
                        color[3] >= 0 ? color[3] : basecol[3]);
    }
    Color4f col;
    Renderer::getColor(ColorMode::MATERIAL, col);
    outcolor = basecol;
    if (outcolor[0] < 0) outcolor[0] = col[0];
    if (outcolor[1] < 0) outcolor[1] = col[1];
    if (outcolor[2] < 0) outcolor[2] = col[2];
    if (outcolor[3] < 0) outcolor[3] = col[3];
    return true;
  }

  return false;
}

size_t VBORenderer::getSurfaceBufferSize(const std::shared_ptr<CSGProducts>& products, bool unique_geometry) const
{
  size_t buffer_size = 0;
  if (unique_geometry) this->geomVisitMark.clear();

  for (const auto& product : products->products) {
    for (const auto& csgobj : product.intersections) {
      buffer_size += getSurfaceBufferSize(csgobj);
    }
    for (const auto& csgobj : product.subtractions) {
      buffer_size += getSurfaceBufferSize(csgobj);
    }
  }
  return buffer_size;
}

size_t VBORenderer::getSurfaceBufferSize(const CSGChainObject& csgobj, bool unique_geometry) const
{
  size_t buffer_size = 0;
  if (unique_geometry && this->geomVisitMark[std::make_pair(csgobj.leaf->polyset.get(), &csgobj.leaf->matrix)]++ > 0) return 0;

  if (csgobj.leaf->polyset) {
    buffer_size += getSurfaceBufferSize(*csgobj.leaf->polyset);
  }
  return buffer_size;
}

size_t VBORenderer::getSurfaceBufferSize(const PolySet& polyset) const
{
  size_t buffer_size = 0;
  for (const auto& poly : polyset.indices) {
    if (poly.size() == 3) {
      buffer_size++;
    } else if (poly.size() == 4) {
      buffer_size += 2;
    } else {
      // poly.size() because we'll render a triangle fan from the centroid
      buffer_size += poly.size();
    }
  }
  return buffer_size * 3;
}

size_t VBORenderer::getEdgeBufferSize(const PolySet& polyset) const
{
  size_t buffer_size = 0;
  for (const auto& polygon : polyset.indices) {
    buffer_size += polygon.size();
  }
  return buffer_size;
}

size_t VBORenderer::getEdgeBufferSize(const Polygon2d& polygon) const
{
  size_t buffer_size = 0;
  // Render only outlines
  for (const Outline2d& o : polygon.outlines()) {
    buffer_size += o.vertices.size();
  }
  return buffer_size;
}

void VBORenderer::add_shader_attributes(VertexArray& vertex_array,
                                        size_t active_point_index, size_t primitive_index,
                                        size_t shape_size, bool outlines) const
{
  if (!shader_attributes_index) return;

  std::shared_ptr<VertexData> vertex_data = vertex_array.data();

  if (getShader().data.csg_rendering.barycentric) {
    // Get edge states
    std::array<GLubyte, 3> barycentric_flags;

    if (!outlines) {
      // top / bottom or 3d object
      if (shape_size == 3) {
        //true, true, true
        barycentric_flags = {0, 0, 0};
      } else if (shape_size == 4) {
        //false, true, true
        barycentric_flags = {1, 0, 0};
      } else {
        //true, false, false
        barycentric_flags = {0, 1, 1};
      }
    } else {
      // sides
      if (primitive_index == 0) {
        //true, false, true
        barycentric_flags = {0, 1, 0};
      } else {
        //true, true, false
        barycentric_flags = {0, 0, 1};
      }
    }

    barycentric_flags[active_point_index] = 1;

    addAttributeValues(*(vertex_data->attributes()[shader_attributes_index + BARYCENTRIC_ATTRIB]), barycentric_flags[0], barycentric_flags[1], barycentric_flags[2], 0);
  }
}

void VBORenderer::create_vertex(VertexArray& vertex_array, const Color4f& color,
                                const std::array<Vector3d, 3>& points,
                                const std::array<Vector3d, 3>& normals,
                                size_t active_point_index, size_t primitive_index,
                                size_t shape_size,
                                bool outlines, bool mirror) const
{
  vertex_array.createVertex(points, normals, color, active_point_index,
                            primitive_index, shape_size, outlines, mirror,
                            [this](VertexArray& vertex_array,
                                   size_t active_point_index, size_t primitive_index,
                                   size_t shape_size, bool outlines) -> void {
    this->add_shader_attributes(vertex_array, active_point_index, primitive_index,
                                shape_size, outlines);
  });
}

void VBORenderer::create_triangle(VertexArray& vertex_array, const Color4f& color,
                                  const Vector3d& p0, const Vector3d& p1, const Vector3d& p2,
                                  size_t primitive_index, size_t shape_size,
                                  bool outlines, bool mirror) const
{
  double ax = p1[0] - p0[0], bx = p1[0] - p2[0];
  double ay = p1[1] - p0[1], by = p1[1] - p2[1];
  double az = p1[2] - p0[2], bz = p1[2] - p2[2];
  double nx = ay * bz - az * by;
  double ny = az * bx - ax * bz;
  double nz = ax * by - ay * bx;
  double nl = sqrt(nx * nx + ny * ny + nz * nz);
  Vector3d n = Vector3d(nx / nl, ny / nl, nz / nl);

  if (!vertex_array.data()) return;

  create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
                0, primitive_index, shape_size,
                outlines, mirror);
  if (!mirror) {
    create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
                  1, primitive_index, shape_size,
                  outlines, mirror);
  }
  create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
                2, primitive_index, shape_size,
                outlines, mirror);
  if (mirror) {
    create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
                  1, primitive_index, shape_size,
                  outlines, mirror);
  }
}

// Since we transform each verted on the CPU, we cache already transformed vertices in the same PolySet
// to avoid redundantly transforming the same vertex value twice, while we process non-indexed PolySets.
Vector3d uniqueMultiply(std::unordered_map<Vector3d, Vector3d>& vert_mult_map,
                               const Vector3d& in_vert, const Transform3d& m)
{
  auto entry = vert_mult_map.find(in_vert);
  if (entry == vert_mult_map.end()) {
    Vector3d out_vert = m * in_vert;
    vert_mult_map.emplace(in_vert, out_vert);
    return out_vert;
  }
  return entry->second;
}

// Creates a VBO "surface" from the PolySet.
// This will usually create a new VertexState and append it to the
// vertex states in the given vertex_array
void VBORenderer::create_surface(const PolySet& ps, VertexArray& vertex_array,
                                 csgmode_e csgmode, const Transform3d& m,
                                 const Color4f& default_color, bool force_default_color) const
{
  std::shared_ptr<VertexData> vertex_data = vertex_array.data();

  if (!vertex_data) {
    return;
  }

  bool mirrored = m.matrix().determinant() < 0;
  size_t triangle_count = 0;

  auto& vertex_states = vertex_array.states();
  std::unordered_map<Vector3d, Vector3d> vert_mult_map;
  size_t last_size = vertex_array.verticesOffset();

  size_t elements_offset = 0;
  if (vertex_array.useElements()) {
    elements_offset = vertex_array.elementsOffset();
    vertex_array.elementsMap().clear();
  }

  auto has_colors = !ps.color_indices.empty();

  for (int i = 0, n = ps.indices.size(); i < n; i++) {
    const auto& poly = ps.indices[i];
    const auto color_index = has_colors && i < ps.color_indices.size() ? ps.color_indices[i] : -1;
    const auto& color = 
      !force_default_color && 
      color_index >= 0 && 
      color_index < ps.colors.size() && 
      ps.colors[color_index].isValid() ? 
      ps.colors[color_index] : default_color;
    if (poly.size() == 3) {
      Vector3d p0 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(0)], m);
      Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(1)], m);
      Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(2)], m);

      create_triangle(vertex_array, color, p0, p1, p2,
                      0, poly.size(), false, mirrored);
      triangle_count++;
    } else if (poly.size() == 4) {
      Vector3d p0 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(0)], m);
      Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(1)], m);
      Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(2)], m);
      Vector3d p3 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(3)], m);

      create_triangle(vertex_array, color, p0, p1, p3,
                      0, poly.size(), false, mirrored);
      create_triangle(vertex_array, color, p2, p3, p1,
                      1, poly.size(), false, mirrored);
      triangle_count += 2;
    } else {
      Vector3d center = Vector3d::Zero();
      for (const auto& idx : poly) {
        center += ps.vertices[idx];
      }
      center /= poly.size();
      for (size_t i = 1; i <= poly.size(); i++) {
        Vector3d p0 = uniqueMultiply(vert_mult_map, center, m);
        Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(i % poly.size())], m);
        Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(i - 1)], m);

        create_triangle(vertex_array, color, p0, p2, p1,
                        i - 1, poly.size(), false, mirrored);
        triangle_count++;
      }
    }
  }

  GLenum elements_type = 0;
  if (vertex_array.useElements()) elements_type = vertex_array.elementsData()->glType();
  std::shared_ptr<VertexState> vs = vertex_array.createVertexState(
    GL_TRIANGLES, triangle_count * 3, elements_type,
    vertex_array.writeIndex(), elements_offset);
  vertex_states.emplace_back(std::move(vs));
  vertex_array.addAttributePointers(last_size);
}

void VBORenderer::create_edges(const Polygon2d& polygon,
                               VertexArray& vertex_array,
                               const Transform3d& m,
                               const Color4f& color) const
{
  std::shared_ptr<VertexData> vertex_data = vertex_array.data();

  if (!vertex_data) return;

  auto& vertex_states = vertex_array.states();
  std::unordered_map<Vector3d, Vector3d> vert_mult_map;

  // Render only outlines
  for (const Outline2d& o : polygon.outlines()) {
    size_t last_size = vertex_array.verticesOffset();
    size_t elements_offset = 0;
    if (vertex_array.useElements()) {
      elements_offset = vertex_array.elementsOffset();
      vertex_array.elementsMap().clear();
    }
    for (const Vector2d& v : o.vertices) {
      Vector3d p0 = uniqueMultiply(vert_mult_map, Vector3d(v[0], v[1], 0.0), m);

      create_vertex(vertex_array, color, {p0}, {}, 0, 0, o.vertices.size(), true, false);
    }

    GLenum elements_type = 0;
    if (vertex_array.useElements()) elements_type = vertex_array.elementsData()->glType();
    std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(
      GL_LINE_LOOP, o.vertices.size(), elements_type,
      vertex_array.writeIndex(), elements_offset);
    vertex_states.emplace_back(std::move(line_loop));
    vertex_array.addAttributePointers(last_size);
  }
}

void VBORenderer::create_polygons(const PolySet& ps, VertexArray& vertex_array,
                                  const Transform3d& m, const Color4f& color) const
{
  assert(ps.getDimension() == 2);
  std::shared_ptr<VertexData> vertex_data = vertex_array.data();

  if (!vertex_data) return;

  auto& vertex_states = vertex_array.states();
  std::unordered_map<Vector3d, Vector3d> vert_mult_map;

  PRINTD("create_polygons 2D");
  bool mirrored = m.matrix().determinant() < 0;
  size_t triangle_count = 0;
  size_t last_size = vertex_array.verticesOffset();
  size_t elements_offset = 0;
  if (vertex_array.useElements()) {
    elements_offset = vertex_array.elementsOffset();
    vertex_array.elementsMap().clear();
  }

  for (const auto& poly : ps.indices) {
    if (poly.size() == 3) {
      Vector3d p0 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(0)], m);
      Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(1)], m);
      Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(2)], m);

      create_triangle(vertex_array, color, p0, p1, p2,
                      0, poly.size(), false, mirrored);
      triangle_count++;
    } else if (poly.size() == 4) {
      Vector3d p0 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(0)], m);
      Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(1)], m);
      Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(2)], m);
      Vector3d p3 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(3)], m);

      create_triangle(vertex_array, color, p0, p1, p3,
                      0, poly.size(), false, mirrored);
      create_triangle(vertex_array, color, p2, p3, p1,
                      1, poly.size(), false, mirrored);
      triangle_count += 2;
    } else {
      Vector3d center = Vector3d::Zero();
      for (const auto& point : poly) {
        center[0] += ps.vertices[point][0];
        center[1] += ps.vertices[point][1];
      }
      center[0] /= poly.size();
      center[1] /= poly.size();

      for (size_t i = 1; i <= poly.size(); i++) {
        Vector3d p0 = uniqueMultiply(vert_mult_map, center, m);
        Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(i % poly.size())], m);
        Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(i - 1)], m);

        create_triangle(vertex_array, color, p0, p2, p1,
                        i - 1, poly.size(), false, mirrored);
        triangle_count++;
      }
    }
  }

  GLenum elements_type = 0;
  if (vertex_array.useElements()) elements_type = vertex_array.elementsData()->glType();
  std::shared_ptr<VertexState> vs = vertex_array.createVertexState(
    GL_TRIANGLES, triangle_count * 3, elements_type,
    vertex_array.writeIndex(), elements_offset);
  vertex_states.emplace_back(std::move(vs));
  vertex_array.addAttributePointers(last_size);
}

void VBORenderer::add_shader_data(VertexArray& vertex_array)
{
  std::shared_ptr<VertexData> vertex_data = vertex_array.data();
  shader_attributes_index = vertex_data->attributes().size();
  vertex_data->addAttributeData(std::make_shared<AttributeData<GLubyte, 4, GL_UNSIGNED_BYTE>>()); // barycentric
}

void VBORenderer::add_shader_pointers(VertexArray& vertex_array)
{
  std::shared_ptr<VertexData> vertex_data = vertex_array.data();

  if (!vertex_data) return;

  size_t start_offset = vertex_array.verticesOffset();

  std::shared_ptr<VertexState> ss = std::make_shared<VBOShaderVertexState>(vertex_array.writeIndex(), 0,
                                                                           vertex_array.verticesVBO(),
                                                                           vertex_array.elementsVBO());
  GLuint index = 0;
  GLsizei count = 0, stride = 0;
  GLenum type = 0;
  size_t offset = 0;

  if (getShader().data.csg_rendering.barycentric) {
    index = getShader().data.csg_rendering.barycentric;
    count = vertex_data->attributes()[shader_attributes_index + BARYCENTRIC_ATTRIB]->count();
    type = vertex_data->attributes()[shader_attributes_index + BARYCENTRIC_ATTRIB]->glType();
    stride = vertex_data->stride();
    offset = start_offset + vertex_data->interleavedOffset(shader_attributes_index + BARYCENTRIC_ATTRIB);
    ss->glBegin().emplace_back([index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
      auto ss = ss_ptr.lock();
      if (ss) {
        // NOLINTBEGIN(performance-no-int-to-ptr)
        GL_TRACE("glVertexAttribPointer(%d, %d, %d, %p)", count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
        GL_CHECKD(glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(ss->drawOffset() + offset)));
        // NOLINTEND(performance-no-int-to-ptr)
      }
    });
  }

  vertex_array.states().emplace_back(std::move(ss));
}

void VBORenderer::add_color(VertexArray& vertex_array, const Color4f& color)
{
  add_shader_pointers(vertex_array);
  shaderinfo_t shader_info = getShader();
  std::shared_ptr<VertexState> color_state = std::make_shared<VBOShaderVertexState>(0, 0, vertex_array.verticesVBO(), vertex_array.elementsVBO());
  color_state->glBegin().emplace_back([shader_info, color]() {
    GL_CHECKD(glUniform4f(shader_info.data.csg_rendering.color_area, color[0], color[1], color[2], color[3]));
    GL_CHECKD(glUniform4f(shader_info.data.csg_rendering.color_edge, (color[0] + 1) / 2, (color[1] + 1) / 2, (color[2] + 1) / 2, 1.0));
  });
  vertex_array.states().emplace_back(std::move(color_state));
}

void VBORenderer::shader_attribs_enable() const
{
  GL_TRACE("glEnableVertexAttribArray(%d)", getShader().data.csg_rendering.barycentric);
  GL_CHECKD(glEnableVertexAttribArray(getShader().data.csg_rendering.barycentric));
}
void VBORenderer::shader_attribs_disable() const
{
  GL_TRACE("glDisableVertexAttribArray(%d)", getShader().data.csg_rendering.barycentric);
  GL_CHECKD(glDisableVertexAttribArray(getShader().data.csg_rendering.barycentric));
}
