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

#include "glview/preview/ThrownTogetherRenderer.h"

#include <cstddef>
#include <memory>
#include <utility>

#include <Eigen/Geometry>

#include "core/enums.h"
#include "geometry/linalg.h"
#include "glview/system-gl.h"
#include "glview/VertexState.h"
#include "glview/Renderer.h"
#include "utils/printutils.h"
#include "core/CSGNode.h"
#include "glview/ShaderUtils.h"
#include "glview/VBOBuilder.h"
#include "glview/VBORenderer.h"

namespace {

Renderer::ColorMode getColorMode(const CSGNode::Flag& flags, bool highlight_mode, bool background_mode,
                                 bool fberror, OpenSCADOperator type)
{
  Renderer::ColorMode colormode = Renderer::ColorMode::NONE;

  if (highlight_mode) {
    colormode = Renderer::ColorMode::HIGHLIGHT;
  } else if (background_mode) {
    if (flags & CSGNode::FLAG_HIGHLIGHT) {
      colormode = Renderer::ColorMode::HIGHLIGHT;
    } else {
      colormode = Renderer::ColorMode::BACKGROUND;
    }
  } else if (fberror) {
  } else if (type == OpenSCADOperator::DIFFERENCE) {
    if (flags & CSGNode::FLAG_HIGHLIGHT) {
      colormode = Renderer::ColorMode::HIGHLIGHT;
    } else {
      colormode = Renderer::ColorMode::CUTOUT;
    }
  } else {
    if (flags & CSGNode::FLAG_HIGHLIGHT) {
      colormode = Renderer::ColorMode::HIGHLIGHT;
    } else {
      colormode = Renderer::ColorMode::MATERIAL;
    }
  }
  return colormode;
}

}  // namespace

ThrownTogetherRenderer::ThrownTogetherRenderer(std::shared_ptr<CSGProducts> root_products,
                                               std::shared_ptr<CSGProducts> highlight_products,
                                               std::shared_ptr<CSGProducts> background_products)
  : root_products_(std::move(root_products)),
    highlight_products_(std::move(highlight_products)),
    background_products_(std::move(background_products))
{
}

void ThrownTogetherRenderer::prepare(const ShaderUtils::ShaderInfo *shaderinfo)
{
  PRINTD("Thrown prepare");
  if (vertex_state_containers_.empty()) {
    VertexStateContainer& vertex_state_container = vertex_state_containers_.emplace_back();

    VBOBuilder vbo_builder(std::make_unique<TTRVertexStateFactory>(), vertex_state_container);
    vbo_builder.addSurfaceData();
    vbo_builder.addShaderData();  // Always enable barycentric coordinates

    size_t num_vertices = 0;
    if (this->root_products_) num_vertices += (calcNumVertices(this->root_products_, true) * 2);
    if (this->background_products_) num_vertices += calcNumVertices(this->background_products_, true);
    if (this->highlight_products_) num_vertices += calcNumVertices(this->highlight_products_, true);

    vbo_builder.allocateBuffers(num_vertices);

    if (this->root_products_)
      createCSGProducts(*this->root_products_, vertex_state_container, vbo_builder, false, false,
                        shaderinfo);
    if (this->background_products_)
      createCSGProducts(*this->background_products_, vertex_state_container, vbo_builder, false, true,
                        shaderinfo);
    if (this->highlight_products_)
      createCSGProducts(*this->highlight_products_, vertex_state_container, vbo_builder, true, false,
                        shaderinfo);

    vbo_builder.createInterleavedVBOs();
  }
}

void ThrownTogetherRenderer::draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo) const
{
  // Only use shader if select rendering or showedges
  const bool enable_shader =
    shaderinfo && ((shaderinfo->type == ShaderUtils::ShaderType::EDGE_RENDERING && showedges) ||
                   shaderinfo->type == ShaderUtils::ShaderType::SELECT_RENDERING);
  if (enable_shader) {
    GL_TRACE("glUseProgram(%d)", shaderinfo->resource.shader_program);
    GL_CHECKD(glUseProgram(shaderinfo->resource.shader_program));
    VBOUtils::shader_attribs_enable(*shaderinfo);
  }

  GL_TRACE0("glDepthFunc(GL_LEQUAL)");
  GL_CHECKD(glDepthFunc(GL_LEQUAL));
  for (const auto& container : vertex_state_containers_) {
    for (const auto& vertex_state : container.states()) {
      // Specify ID color if we're using select rendering
      if (shaderinfo && shaderinfo->type == ShaderUtils::ShaderType::SELECT_RENDERING) {
        if (const auto ttr_vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_state)) {
          GL_TRACE("glUniform3f(%d, %f, %f, %f)",
                   shaderinfo->uniforms.at("frag_idcolor") %
                     (((ttr_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f) %
                     (((ttr_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f) %
                     (((ttr_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
          GL_CHECKD(glUniform3f(shaderinfo->uniforms.at("frag_idcolor"),
                                ((ttr_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f,
                                ((ttr_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f,
                                ((ttr_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
        }
      }
      const auto shader_vs = std::dynamic_pointer_cast<VBOShaderVertexState>(vertex_state);
      if (!shader_vs || (shader_vs && showedges)) {
        vertex_state->draw();
      }
    }
  }

  if (enable_shader) {
    VBOUtils::shader_attribs_disable(*shaderinfo);
    glUseProgram(0);
  }
}

void ThrownTogetherRenderer::createChainObject(VertexStateContainer& container, VBOBuilder& vbo_builder,
                                               const CSGChainObject& csgobj, bool highlight_mode,
                                               bool background_mode, OpenSCADOperator type,
                                               const ShaderUtils::ShaderInfo *shaderinfo)
{
  if (!csgobj.leaf->polyset ||
      this->geom_visit_mark_[std::make_pair(csgobj.leaf->polyset.get(), &csgobj.leaf->matrix)]++ > 0) {
    return;
  }

  const bool enable_barycentric = true;

  const auto& leaf_color = csgobj.leaf->color;

  vbo_builder.writeSurface();

  Color4f color;
  if (highlight_mode || background_mode) {
    const ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
    getShaderColor(colormode, leaf_color, color);

    add_shader_pointers(vbo_builder, shaderinfo);

    vbo_builder.create_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, color, enable_barycentric);
    if (const auto ttr_vs = std::dynamic_pointer_cast<TTRVertexState>(vbo_builder.states().back())) {
      ttr_vs->setCsgObjectIndex(csgobj.leaf->index);
    }
  } else {  // root mode
    ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
    getShaderColor(colormode, leaf_color, color);

    add_shader_pointers(vbo_builder, shaderinfo);

    auto cull = std::make_shared<VertexState>();
    cull->glBegin().emplace_back([]() {
      GL_TRACE0("glEnable(GL_CULL_FACE)");
      GL_CHECKD(glEnable(GL_CULL_FACE));
      GL_TRACE0("glCullFace(GL_BACK)");
      GL_CHECKD(glCullFace(GL_BACK));
    });
    container.states().emplace_back(std::move(cull));

    Transform3d mat = csgobj.leaf->matrix;
    if (csgobj.leaf->polyset->getDimension() == 2 && type == OpenSCADOperator::DIFFERENCE) {
      // Scale 2D negative objects 10% in the Z direction to avoid z fighting
      mat *= Eigen::Scaling(1.0, 1.0, 1.1);
    }
    vbo_builder.create_surface(*csgobj.leaf->polyset, mat, color, enable_barycentric);
    if (auto ttr_vs = std::dynamic_pointer_cast<TTRVertexState>(vbo_builder.states().back())) {
      ttr_vs->setCsgObjectIndex(csgobj.leaf->index);
    }

    color.setRgb(1.0f, 0.0f, 1.0f);  // override leaf color on front/back error

    colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, true, type);
    getShaderColor(colormode, leaf_color, color);

    add_shader_pointers(vbo_builder, shaderinfo);

    cull = std::make_shared<VertexState>();
    cull->glBegin().emplace_back([]() {
      GL_TRACE0("glCullFace(GL_FRONT)");
      GL_CHECKD(glCullFace(GL_FRONT));
    });
    container.states().emplace_back(std::move(cull));

    vbo_builder.create_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, color, enable_barycentric);
    if (auto ttr_vs = std::dynamic_pointer_cast<TTRVertexState>(vbo_builder.states().back())) {
      ttr_vs->setCsgObjectIndex(csgobj.leaf->index);
    }

    container.states().back()->glEnd().emplace_back([]() {
      GL_TRACE0("glDisable(GL_CULL_FACE)");
      GL_CHECKD(glDisable(GL_CULL_FACE));
    });
  }
}

void ThrownTogetherRenderer::createCSGProducts(const CSGProducts& products,
                                               VertexStateContainer& container, VBOBuilder& vbo_builder,
                                               bool highlight_mode, bool background_mode,
                                               const ShaderUtils::ShaderInfo *shaderinfo)
{
  PRINTD("Thrown renderCSGProducts");
  this->geom_visit_mark_.clear();

  for (const auto& product : products.products) {
    for (const auto& csgobj : product.intersections) {
      createChainObject(container, vbo_builder, csgobj, highlight_mode, background_mode,
                        OpenSCADOperator::INTERSECTION, shaderinfo);
    }
    for (const auto& csgobj : product.subtractions) {
      createChainObject(container, vbo_builder, csgobj, highlight_mode, background_mode,
                        OpenSCADOperator::DIFFERENCE, shaderinfo);
    }
  }
}

BoundingBox ThrownTogetherRenderer::getBoundingBox() const
{
  BoundingBox bbox;
  if (this->root_products_) {
    bbox = this->root_products_->getBoundingBox(true);
  }
  if (this->highlight_products_) {
    bbox.extend(this->highlight_products_->getBoundingBox(true));
  }
  if (this->background_products_) {
    bbox.extend(this->background_products_->getBoundingBox(true));
  }
  return bbox;
}
