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
#include "utils/hash.h"  // IWYU pragma: keep

#include <cassert>
#include <array>
#include <unordered_map>
#include <utility>
#include <memory>
#include <cstddef>

namespace VBOUtils {

void shader_attribs_enable(const ShaderUtils::ShaderInfo& shaderinfo)
{
  for (const auto& [name, location] : shaderinfo.attributes) {
    GL_TRACE("glEnableVertexAttribArray(%d)", location);
    GL_CHECKD(glEnableVertexAttribArray(location));
  }
}

void shader_attribs_disable(const ShaderUtils::ShaderInfo& shaderinfo)
{
  for (const auto& [name, location] : shaderinfo.attributes) {
    GL_TRACE("glEnableVertexAttribArray(%d)", location);
    GL_CHECKD(glDisableVertexAttribArray(location));
  }
}

}  // namespace VBOUtils

VBORenderer::VBORenderer() : Renderer() {}

// TODO: Move to Renderer?
bool VBORenderer::getShaderColor(Renderer::ColorMode colormode, const Color4f& color,
                                 Color4f& outcolor) const
{
  if ((colormode != ColorMode::BACKGROUND) && (colormode != ColorMode::HIGHLIGHT) && color.isValid()) {
    outcolor = color;
    return true;
  }
  Color4f basecol;
  if (Renderer::getColor(colormode, basecol)) {
    if (colormode == ColorMode::BACKGROUND || colormode != ColorMode::HIGHLIGHT) {
      basecol = Color4f(color[0] >= 0 ? color[0] : basecol[0], color[1] >= 0 ? color[1] : basecol[1],
                        color[2] >= 0 ? color[2] : basecol[2], color[3] >= 0 ? color[3] : basecol[3]);
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

size_t VBORenderer::calcNumVertices(const std::shared_ptr<CSGProducts>& products,
                                         bool unique_geometry) const
{
  size_t buffer_size = 0;
  if (unique_geometry) this->geom_visit_mark_.clear();

  for (const auto& product : products->products) {
    for (const auto& csgobj : product.intersections) {
      buffer_size += calcNumVertices(csgobj);
    }
    for (const auto& csgobj : product.subtractions) {
      buffer_size += calcNumVertices(csgobj);
    }
  }
  return buffer_size;
}

size_t VBORenderer::calcNumVertices(const CSGChainObject& csgobj, bool unique_geometry) const
{
  size_t buffer_size = 0;
  if (unique_geometry &&
      this->geom_visit_mark_[std::make_pair(csgobj.leaf->polyset.get(), &csgobj.leaf->matrix)]++ > 0)
    return 0;

  if (csgobj.leaf->polyset) {
    buffer_size += calcNumVertices(*csgobj.leaf->polyset);
  }
  return buffer_size;
}

size_t VBORenderer::calcNumVertices(const PolySet& polyset) const
{
  size_t buffer_size = 0;
  for (const auto& poly : polyset.indices) {
    if (poly.size() == 3) {
      buffer_size++;
    } else if (poly.size() == 4) {
      buffer_size += 2;
    } else {
      // poly.size() because we'll render a triangle fan from the centroid
      // FIXME: Are we still using this code path?
      buffer_size += poly.size();
    }
  }
  return buffer_size * 3;
}

size_t VBORenderer::calcNumEdgeVertices(const PolySet& polyset) const
{
  size_t buffer_size = 0;
  for (const auto& polygon : polyset.indices) {
    buffer_size += polygon.size();
  }
  return buffer_size;
}

size_t VBORenderer::calcNumEdgeVertices(const Polygon2d& polygon) const
{
  size_t buffer_size = 0;
  // Render only outlines
  for (const Outline2d& o : polygon.outlines()) {
    buffer_size += o.vertices.size();
  }
  return buffer_size;
}

// Since we transform each verted on the CPU, we cache already transformed vertices in the same PolySet
// to avoid redundantly transforming the same vertex value twice, while we process non-indexed PolySets.
Vector3d uniqueMultiply(std::unordered_map<Vector3d, Vector3d>& vert_mult_map, const Vector3d& in_vert,
                        const Transform3d& m)
{
  auto entry = vert_mult_map.find(in_vert);
  if (entry == vert_mult_map.end()) {
    Vector3d out_vert = m * in_vert;
    vert_mult_map.emplace(in_vert, out_vert);
    return out_vert;
  }
  return entry->second;
}

void VBORenderer::create_edges(const Polygon2d& polygon,
                               VBOBuilder& vbo_builder,
                               const Transform3d& m,
                               const Color4f& color) const
{
  const std::shared_ptr<VertexData> vertex_data = vbo_builder.data();

  if (!vertex_data) return;

  auto& vertex_states = vbo_builder.states();
  std::unordered_map<Vector3d, Vector3d> vert_mult_map;

  // Render only outlines
  for (const Outline2d& o : polygon.outlines()) {
    const auto last_size = vbo_builder.verticesOffset();
    size_t elements_offset = 0;
    if (vbo_builder.useElements()) {
      elements_offset = vbo_builder.elementsOffset();
      vbo_builder.elementsMap().clear();
    }
    for (const Vector2d& v : o.vertices) {
      const Vector3d p0 = uniqueMultiply(vert_mult_map, Vector3d(v[0], v[1], 0.0), m);
      vbo_builder.createVertex({p0}, {}, color, 0, 0, o.vertices.size(), true, false);
    }

    GLenum elements_type = 0;
    if (vbo_builder.useElements()) elements_type = vbo_builder.elementsData()->glType();
    std::shared_ptr<VertexState> line_loop = vbo_builder.createVertexState(
      GL_LINE_LOOP, o.vertices.size(), elements_type, vbo_builder.writeIndex(), elements_offset);
    vertex_states.emplace_back(std::move(line_loop));
    vbo_builder.addAttributePointers(last_size);
  }
}

void VBORenderer::create_polygons(const PolySet& ps, VBOBuilder& vbo_builder, const Transform3d& m,
                                  const Color4f& color) const
{
  assert(ps.getDimension() == 2);
  const std::shared_ptr<VertexData> vertex_data = vbo_builder.data();

  if (!vertex_data) return;

  auto& vertex_states = vbo_builder.states();
  std::unordered_map<Vector3d, Vector3d> vert_mult_map;

  PRINTD("create_polygons 2D");
  const bool mirrored = m.matrix().determinant() < 0;
  size_t triangle_count = 0;
  const auto last_size = vbo_builder.verticesOffset();
  size_t elements_offset = 0;
  if (vbo_builder.useElements()) {
    elements_offset = vbo_builder.elementsOffset();
    vbo_builder.elementsMap().clear();
  }

  for (const auto& poly : ps.indices) {
    if (poly.size() == 3) {
      const Vector3d p0 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(0)], m);
      const Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(1)], m);
      const Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(2)], m);

      vbo_builder.create_triangle(color, p0, p1, p2, 0, poly.size(), false, false, mirrored);
      triangle_count++;
    } else if (poly.size() == 4) {
      const Vector3d p0 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(0)], m);
      const Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(1)], m);
      const Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(2)], m);
      const Vector3d p3 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(3)], m);

      vbo_builder.create_triangle(color, p0, p1, p3, 0, poly.size(), false, false, mirrored);
      vbo_builder.create_triangle(color, p2, p3, p1, 1, poly.size(), false, false, mirrored);
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
        const Vector3d p0 = uniqueMultiply(vert_mult_map, center, m);
        const Vector3d p1 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(i % poly.size())], m);
        const Vector3d p2 = uniqueMultiply(vert_mult_map, ps.vertices[poly.at(i - 1)], m);

        vbo_builder.create_triangle(color, p0, p2, p1, i - 1, poly.size(), false, false, mirrored);
        triangle_count++;
      }
    }
  }

  GLenum elements_type = 0;
  if (vbo_builder.useElements()) elements_type = vbo_builder.elementsData()->glType();
  std::shared_ptr<VertexState> vs = vbo_builder.createVertexState(
    GL_TRIANGLES, triangle_count * 3, elements_type, vbo_builder.writeIndex(), elements_offset);
  vertex_states.emplace_back(std::move(vs));
  vbo_builder.addAttributePointers(last_size);
}

void VBORenderer::add_shader_pointers(VBOBuilder& vbo_builder, const ShaderUtils::ShaderInfo *shaderinfo)
{
  const std::shared_ptr<VertexData> vertex_data = vbo_builder.data();

  if (!vertex_data) return;

  const auto start_offset = vbo_builder.verticesOffset();

  std::shared_ptr<VertexState> ss = std::make_shared<VBOShaderVertexState>(
    vbo_builder.writeIndex(), 0, vbo_builder.verticesVBO(), vbo_builder.elementsVBO());
  GLsizei count = 0, stride = 0;
  GLenum type = 0;
  size_t offset = 0;
  GLuint index = shaderinfo->attributes.at("barycentric");

  if (index > 0) {
    count =
      vertex_data->attributes()[vbo_builder.shader_attributes_index_ + BARYCENTRIC_ATTRIB]->count();
    type =
      vertex_data->attributes()[vbo_builder.shader_attributes_index_ + BARYCENTRIC_ATTRIB]->glType();
    stride = vertex_data->stride();
    offset = start_offset +
             vertex_data->interleavedOffset(vbo_builder.shader_attributes_index_ + BARYCENTRIC_ATTRIB);
    ss->glBegin().emplace_back(
      [index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
        auto ss = ss_ptr.lock();
        if (ss) {
          // NOLINTBEGIN(performance-no-int-to-ptr)
          GL_TRACE("glVertexAttribPointer(%d, %d, %d, %d, %p)",
                   index % count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
          GL_CHECKD(glVertexAttribPointer(index, count, type, GL_FALSE, stride,
                                          (GLvoid *)(ss->drawOffset() + offset)));
          // NOLINTEND(performance-no-int-to-ptr)
        }
      });
  }

  vbo_builder.states().emplace_back(std::move(ss));
}

// This will set the `color_area` and `color_edge` shader uniforms.
// ..meaning it will only be applicable to shaders using those uniforms, i.e. the edge shader
void VBORenderer::add_color(VBOBuilder& vbo_builder, const Color4f& color, const ShaderUtils::ShaderInfo *shaderinfo)
{
  if (!shaderinfo) return;
  add_shader_pointers(vbo_builder, shaderinfo);
  std::shared_ptr<VertexState> color_state =
    std::make_shared<VBOShaderVertexState>(0, 0, vbo_builder.verticesVBO(), vbo_builder.elementsVBO());
  color_state->glBegin().emplace_back([shaderinfo, color]() {
    GL_TRACE("glUniform4f(%d, %.2f, %.2f, %.2f, %.2f)", shaderinfo->uniforms.at("color_area") % color[0] % color[1] % color[2] % color[3]);
    GL_CHECKD(
      glUniform4f(shaderinfo->uniforms.at("color_area"), color[0], color[1], color[2], color[3]));
    GL_TRACE("glUniform4f(%d, %.2f, %.2f, %.2f, %.2f)", shaderinfo->uniforms.at("color_edge") % ((color[0] + 1) / 2) % ((color[1] + 1) / 2) %
                          ((color[2] + 1) / 2) % 1.0);
    GL_CHECKD(glUniform4f(shaderinfo->uniforms.at("color_edge"), (color[0] + 1) / 2, (color[1] + 1) / 2,
                          (color[2] + 1) / 2, 1.0));
  });
  vbo_builder.states().emplace_back(std::move(color_state));
}
