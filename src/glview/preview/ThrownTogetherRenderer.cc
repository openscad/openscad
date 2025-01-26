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

#include <memory>
#include <cstddef>
#include <utility>
#include "Feature.h"
#include "VertexState.h"
#include "geometry/PolySet.h"
#include "core/enums.h"
#include "utils/printutils.h"

#include "glview/system-gl.h"

namespace {

Renderer::ColorMode getColorMode(const CSGNode::Flag& flags, bool highlight_mode,
                       bool background_mode, bool fberror, OpenSCADOperator type) {
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
  : root_products_(std::move(root_products)), highlight_products_(std::move(highlight_products)), background_products_(std::move(background_products))
{
}

ThrownTogetherRenderer::~ThrownTogetherRenderer()
{
}

void ThrownTogetherRenderer::prepare(bool /*showedges*/, const RendererUtils::ShaderInfo *shaderinfo)
{
  PRINTD("Thrown prepare");
  if (vertex_state_containers_.empty()) {
    vertex_state_containers_.emplace_back();
    VertexStateContainer &vertex_state_container = vertex_state_containers_.back();

    VBOBuilder vertex_array(std::make_unique<TTRVertexStateFactory>(), 
                            vertex_state_container.vertex_states_, 
                            vertex_state_container.vertices_vbo_, 
                            vertex_state_container.elements_vbo_);
    vertex_array.addSurfaceData();
    bool enable_barycentric = shaderinfo && shaderinfo->attributes.at("barycentric");
    if (enable_barycentric) {
      vertex_array.addShaderData();
    } else {
      LOG("Warning: Shader not available");
    }

    size_t num_vertices = 0;
    if (this->root_products_) num_vertices += (calcNumVertices(this->root_products_, true) * 2);
    if (this->background_products_) num_vertices += calcNumVertices(this->background_products_, true);
    if (this->highlight_products_) num_vertices += calcNumVertices(this->highlight_products_, true);

    vertex_array.allocateBuffers(num_vertices);

    if (this->root_products_) createCSGProducts(*this->root_products_, vertex_state_container, vertex_array, false, false, shaderinfo);
    if (this->background_products_) createCSGProducts(*this->background_products_, vertex_state_container, vertex_array, false, true, shaderinfo);
    if (this->highlight_products_) createCSGProducts(*this->highlight_products_, vertex_state_container, vertex_array, true, false, shaderinfo);

    vertex_array.createInterleavedVBOs();
  }
}


void ThrownTogetherRenderer::draw(bool showedges, const RendererUtils::ShaderInfo *shaderinfo) const
{
  if (shaderinfo) {
    glUseProgram(shaderinfo->shader_program);
    if (shaderinfo->type == RendererUtils::ShaderType::EDGE_RENDERING && showedges) {
      VBOUtils::shader_attribs_enable(*shaderinfo);
    }
  }

  renderCSGProducts(std::make_shared<CSGProducts>(), showedges, shaderinfo);
  
  if (shaderinfo) {
    if (shaderinfo->type == RendererUtils::ShaderType::EDGE_RENDERING && showedges) {
      VBOUtils::shader_attribs_disable(*shaderinfo);
    }
    glUseProgram(0);
  }
}

void ThrownTogetherRenderer::renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges,
                                               const RendererUtils::ShaderInfo *shaderinfo,
                                               bool highlight_mode, bool background_mode,
                                               bool fberror) const
{
  PRINTD("Thrown renderCSGProducts");
  glDepthFunc(GL_LEQUAL);
  this->geom_visit_mark_.clear();
  for (const auto& container : vertex_state_containers_) {
    for (const auto& vs : container.vertex_states_) {
      if (vs) {
        if (const auto csg_vs = std::dynamic_pointer_cast<TTRVertexState>(vs)) {
          if (shaderinfo && shaderinfo->type == RendererUtils::ShaderType::SELECT_RENDERING) {
            GL_TRACE("glUniform3f(%d, %f, %f, %f)",
                    shaderinfo->uniforms.at("frag_idcolor") %
                    (((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f) %
                    (((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f) %
                    (((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
            GL_CHECKD(glUniform3f(shaderinfo->uniforms.at("frag_idcolor"),
                                  ((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f,
                                  ((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f,
                                  ((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
          }
        }
        const auto shader_vs = std::dynamic_pointer_cast<VBOShaderVertexState>(vs);
        if (!shader_vs || (shader_vs && showedges)) {
          vs->draw();
        }
      }
    }
  }
}

void ThrownTogetherRenderer::createChainObject(VertexStateContainer& container, VBOBuilder& vertex_array,
                                               const CSGChainObject& csgobj, bool highlight_mode,
                                               bool background_mode, OpenSCADOperator type, const RendererUtils::ShaderInfo *shaderinfo)
{
  if (!csgobj.leaf->polyset ||
      this->geom_visit_mark_[std::make_pair(csgobj.leaf->polyset.get(), &csgobj.leaf->matrix)]++ > 0) {
    return;
  }

  bool enable_barycentric = shaderinfo && shaderinfo->attributes.at("barycentric");

  const auto& leaf_color = csgobj.leaf->color;
  const auto csgmode = RendererUtils::getCsgMode(highlight_mode, background_mode, type);

  vertex_array.writeSurface();

  Color4f color;
  if (highlight_mode || background_mode) {
    const ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
    getShaderColor(colormode, leaf_color, color);

    add_color(vertex_array, color, shaderinfo);

    vertex_array.create_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, color, enable_barycentric);
    if (const auto vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back())) {
      vs->setCsgObjectIndex(csgobj.leaf->index);
    }
  } else { // root mode
    ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
    getShaderColor(colormode, leaf_color, color);

    add_color(vertex_array, color, shaderinfo);

    auto cull = std::make_shared<VertexState>();
    cull->glBegin().emplace_back([]() {
      GL_TRACE0("glEnable(GL_CULL_FACE)");
      GL_CHECKD(glEnable(GL_CULL_FACE));
      GL_TRACE0("glCullFace(GL_BACK)");
      GL_CHECKD(glCullFace(GL_BACK));
    });
    container.vertex_states_.emplace_back(std::move(cull));

    Transform3d mat = csgobj.leaf->matrix;
    if (csgobj.leaf->polyset->getDimension() == 2 && type == OpenSCADOperator::DIFFERENCE) {
      // Scale 2D negative objects 10% in the Z direction to avoid z fighting
      mat *= Eigen::Scaling(1.0, 1.0, 1.1);
    }
    vertex_array.create_surface(*csgobj.leaf->polyset, mat, color, enable_barycentric);
    if (auto vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back())) {
      vs->setCsgObjectIndex(csgobj.leaf->index);
    }

    color[0] = 1.0; color[1] = 0.0; color[2] = 1.0; // override leaf color on front/back error

    colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, true, type);
    getShaderColor(colormode, leaf_color, color);

    add_color(vertex_array, color, shaderinfo);

    cull = std::make_shared<VertexState>();
    cull->glBegin().emplace_back([]() {
      GL_TRACE0("glCullFace(GL_FRONT)");
      GL_CHECKD(glCullFace(GL_FRONT));
    });
    container.vertex_states_.emplace_back(std::move(cull));

    vertex_array.create_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, color, enable_barycentric);
    if (auto vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back())) {
      vs->setCsgObjectIndex(csgobj.leaf->index);
    }

    container.vertex_states_.back()->glEnd().emplace_back([]() {
      GL_TRACE0("glDisable(GL_CULL_FACE)");
      GL_CHECKD(glDisable(GL_CULL_FACE));
    });
  }
}

void ThrownTogetherRenderer::createCSGProducts(const CSGProducts& products, VertexStateContainer& container, VBOBuilder& vertex_array,
                                               bool highlight_mode, bool background_mode, const RendererUtils::ShaderInfo *shaderinfo)
{
  PRINTD("Thrown renderCSGProducts");
  this->geom_visit_mark_.clear();

  for (const auto& product : products.products) {
    for (const auto& csgobj : product.intersections) {
      createChainObject(container, vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION, shaderinfo);
    }
    for (const auto& csgobj : product.subtractions) {
      createChainObject(container, vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE, shaderinfo);
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
