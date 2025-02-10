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
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
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

  GLuint attribute_index = shaderinfo->attributes.at("barycentric");
  if (attribute_index > 0) {
    count =
      vertex_data->attributes()[vbo_builder.shader_attributes_index_ + BARYCENTRIC_ATTRIB]->count();
    type =
      vertex_data->attributes()[vbo_builder.shader_attributes_index_ + BARYCENTRIC_ATTRIB]->glType();
    stride = vertex_data->stride();
    offset = start_offset +
             vertex_data->interleavedOffset(vbo_builder.shader_attributes_index_ + BARYCENTRIC_ATTRIB);
    ss->glBegin().emplace_back(
      [attribute_index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
        auto ss = ss_ptr.lock();
        if (ss) {
          // NOLINTBEGIN(performance-no-int-to-ptr)
          GL_TRACE("glVertexAttribPointer(%d, %d, %d, GL_FALSE, %d, %p)",
                   attribute_index % count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
          GL_CHECKD(glVertexAttribPointer(attribute_index, count, type, GL_FALSE, stride,
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
