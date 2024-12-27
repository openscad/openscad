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
  if (vertices_vbo_) {
    glDeleteBuffers(1, &vertices_vbo_);
  }
  if (elements_vbo_) {
    glDeleteBuffers(1, &elements_vbo_);
  }
}

void ThrownTogetherRenderer::prepare(bool /*showfaces*/, bool /*showedges*/, const Renderer::ShaderInfo * /*shaderinfo*/)
{
  PRINTD("Thrown prepare");
  if (vertex_states_.empty()) {
    glGenBuffers(1, &vertices_vbo_);
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      glGenBuffers(1, &elements_vbo_);
    }
    VertexArray vertex_array(std::make_unique<TTRVertexStateFactory>(), vertex_states_, vertices_vbo_, elements_vbo_);
    vertex_array.addSurfaceData();
    if (getShader().progid != 0) {
      add_shader_data(vertex_array);
    } else {
      LOG("Warning: Shader not available");
    }

    size_t num_vertices = 0;
    if (this->root_products_) num_vertices += (getSurfaceBufferSize(this->root_products_, true) * 2);
    if (this->background_products_) num_vertices += getSurfaceBufferSize(this->background_products_, true);
    if (this->highlight_products_) num_vertices += getSurfaceBufferSize(this->highlight_products_, true);

    vertex_array.allocateBuffers(num_vertices);

    if (this->root_products_) createCSGProducts(*this->root_products_, vertex_array, false, false);
    if (this->background_products_) createCSGProducts(*this->background_products_, vertex_array, false, true);
    if (this->highlight_products_) createCSGProducts(*this->highlight_products_, vertex_array, true, false);

    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));

    vertex_array.createInterleavedVBOs();
  }
}


void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges, const Renderer::ShaderInfo *shaderinfo) const
{
  PRINTD("draw()");
  if (!shaderinfo && showedges) {
    shaderinfo = &getShader();
  }
  if (shaderinfo && shaderinfo->progid) {
    glUseProgram(shaderinfo->progid);
    if (shaderinfo->type == ShaderType::EDGE_RENDERING && showedges) {
      shader_attribs_enable();
    }
  }

  renderCSGProducts(std::make_shared<CSGProducts>(), showedges, shaderinfo);
  if (shaderinfo && shaderinfo->progid) {
    if (shaderinfo->type == ShaderType::EDGE_RENDERING && showedges) {
      shader_attribs_disable();
    }
    glUseProgram(0);
  }
}

void ThrownTogetherRenderer::renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges,
                                               const Renderer::ShaderInfo *shaderinfo,
                                               bool highlight_mode, bool background_mode,
                                               bool fberror) const
{
  PRINTD("Thrown renderCSGProducts");
  glDepthFunc(GL_LEQUAL);
  this->geom_visit_mark_.clear();

  for (const auto& vs : vertex_states_) {
    if (vs) {
      if (const auto csg_vs = std::dynamic_pointer_cast<TTRVertexState>(vs)) {
        if (shaderinfo && shaderinfo->type == Renderer::ShaderType::SELECT_RENDERING) {
          GL_TRACE("glUniform3f(%d, %f, %f, %f)",
                   shaderinfo->data.select_rendering.identifier %
                   (((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f) %
                   (((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f) %
                   (((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
          GL_CHECKD(glUniform3f(shaderinfo->data.select_rendering.identifier,
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

void ThrownTogetherRenderer::createChainObject(VertexArray& vertex_array,
                                               const CSGChainObject& csgobj, bool highlight_mode,
                                               bool background_mode, OpenSCADOperator type)
{
  if (!csgobj.leaf->polyset ||
      this->geom_visit_mark_[std::make_pair(csgobj.leaf->polyset.get(), &csgobj.leaf->matrix)]++ > 0) {
    return;
  }

  const auto& leaf_color = csgobj.leaf->color;
  const auto csgmode = RendererUtils::getCsgMode(highlight_mode, background_mode, type);

  vertex_array.writeSurface();

  Color4f color;
  if (highlight_mode || background_mode) {
    const ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
    getShaderColor(colormode, leaf_color, color);

    add_color(vertex_array, color);

    create_surface(*csgobj.leaf->polyset, vertex_array, csgmode, csgobj.leaf->matrix, color);
    if (const auto vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back())) {
      vs->setCsgObjectIndex(csgobj.leaf->index);
    }
  } else { // root mode
    ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
    getShaderColor(colormode, leaf_color, color);

    add_color(vertex_array, color);

    auto cull = std::make_shared<VertexState>();
    cull->glBegin().emplace_back([]() {
      GL_TRACE0("glEnable(GL_CULL_FACE)");
      GL_CHECKD(glEnable(GL_CULL_FACE));
      GL_TRACE0("glCullFace(GL_BACK)");
      GL_CHECKD(glCullFace(GL_BACK));
    });
    vertex_states_.emplace_back(std::move(cull));

    Transform3d mat = csgobj.leaf->matrix;
    if (csgobj.leaf->polyset->getDimension() == 2 && type == OpenSCADOperator::DIFFERENCE) {
      // Scale 2D negative objects 10% in the Z direction to avoid z fighting
      mat *= Eigen::Scaling(1.0, 1.0, 1.1);
    }
    create_surface(*csgobj.leaf->polyset, vertex_array, csgmode, mat, color);
    if (auto vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back())) {
      vs->setCsgObjectIndex(csgobj.leaf->index);
    }

    color[0] = 1.0; color[1] = 0.0; color[2] = 1.0; // override leaf color on front/back error

    colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, true, type);
    getShaderColor(colormode, leaf_color, color);

    add_color(vertex_array, color);

    cull = std::make_shared<VertexState>();
    cull->glBegin().emplace_back([]() {
      GL_TRACE0("glCullFace(GL_FRONT)");
      GL_CHECKD(glCullFace(GL_FRONT));
    });
    vertex_states_.emplace_back(std::move(cull));

    create_surface(*csgobj.leaf->polyset, vertex_array, csgmode, csgobj.leaf->matrix, color);
    if (auto vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back())) {
      vs->setCsgObjectIndex(csgobj.leaf->index);
    }

    vertex_states_.back()->glEnd().emplace_back([]() {
      GL_TRACE0("glDisable(GL_CULL_FACE)");
      GL_CHECKD(glDisable(GL_CULL_FACE));
    });
  }
}

void ThrownTogetherRenderer::createCSGProducts(const CSGProducts& products, VertexArray& vertex_array,
                                               bool highlight_mode, bool background_mode)
{
  PRINTD("Thrown renderCSGProducts");
  this->geom_visit_mark_.clear();

  for (const auto& product : products.products) {
    for (const auto& csgobj : product.intersections) {
      createChainObject(vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION);
    }
    for (const auto& csgobj : product.subtractions) {
      createChainObject(vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);
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
