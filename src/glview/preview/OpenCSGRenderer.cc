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

#include "glview/preview/OpenCSGRenderer.h"
#include "geometry/linalg.h"
#include "glview/system-gl.h"

#include "Feature.h"
#include "geometry/PolySet.h"
#include <cassert>
#include <memory>
#include <memory.h>
#include <cstddef>
#include <utility>
#include <vector>

#ifdef ENABLE_OPENCSG

class OpenCSGVBOPrim : public OpenCSG::Primitive {
public:
  OpenCSGVBOPrim(OpenCSG::Operation operation, unsigned int convexity,
                 std::unique_ptr<VertexState> vertex_state)
      : OpenCSG::Primitive(operation, convexity),
        vertex_state(std::move(vertex_state)) {}
  void render() override {
    if (vertex_state != nullptr) {
      vertex_state->draw();
    } else {
      if (OpenSCAD::debug != "") PRINTD("OpenCSGVBOPrim vertex_state was null");
    }
  }

private:
  const std::unique_ptr<VertexState> vertex_state;
};
#endif // ENABLE_OPENCSG

OpenCSGRenderer::OpenCSGRenderer(
    std::shared_ptr<CSGProducts> root_products,
    std::shared_ptr<CSGProducts> highlights_products,
    std::shared_ptr<CSGProducts> background_products)
    : root_products_(std::move(root_products)),
      highlights_products_(std::move(highlights_products)),
      background_products_(std::move(background_products)) {}

void OpenCSGRenderer::prepare(bool /*showfaces*/, bool /*showedges*/,
                              const shaderinfo_t *shaderinfo) {
  if (vbo_vertex_products_.empty()) {
    if (root_products_) {
      createCSGVBOProducts(*root_products_, shaderinfo, false, false);
    }
    if (background_products_) {
      createCSGVBOProducts(*background_products_, shaderinfo, false, true);
    }
    if (highlights_products_) {
      createCSGVBOProducts(*highlights_products_, shaderinfo, true, false);
    }
  }
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges,
                           const shaderinfo_t *shaderinfo) const {
  if (!shaderinfo && showedges) shaderinfo = &getShader();
  renderCSGVBOProducts(showedges, shaderinfo);
}

// Primitive for drawing using OpenCSG
// Makes a copy of the given VertexState enabling just unlit/uncolored vertex
// rendering
OpenCSGVBOPrim *OpenCSGRenderer::createVBOPrimitive(
    const std::shared_ptr<OpenCSGVertexState> &vertex_state,
    const OpenCSG::Operation operation, const unsigned int convexity) const {
  std::unique_ptr<VertexState> opencsg_vs = std::make_unique<VertexState>(
      vertex_state->drawMode(), vertex_state->drawSize(),
      vertex_state->drawType(), vertex_state->drawOffset(),
      vertex_state->elementOffset(), vertex_state->verticesVBO(),
      vertex_state->elementsVBO());
  // First two glBegin entries are the vertex position calls
  opencsg_vs->glBegin().insert(opencsg_vs->glBegin().begin(),
                               vertex_state->glBegin().begin(),
                               vertex_state->glBegin().begin() + 2);
  // First glEnd entry is the disable vertex position call
  opencsg_vs->glEnd().insert(opencsg_vs->glEnd().begin(),
                             vertex_state->glEnd().begin(),
                             vertex_state->glEnd().begin() + 1);

  return new OpenCSGVBOPrim(operation, convexity, std::move(opencsg_vs));
}

// Turn the CSGProducts into VBOs
// Will create one (temporary) VertexArray and one VBO(+EBO) per product
// The VBO will be utilized to render multiple objects with correct state
// management. In the future, we could use a VBO per primitive and potentially
// reuse VBOs, but that requires some more careful state management.
// Note: This function can be called multiple times for different products.
// Each call will add to vbo_vertex_products_.
void OpenCSGRenderer::createCSGVBOProducts(
    const CSGProducts &products, const Renderer::shaderinfo_t * /*shaderinfo*/,
    bool highlight_mode, bool background_mode) {
  // We need to manage buffers here since we don't have another suitable
  // container for managing the life cycle of VBOs. We're creating one VBO(+EBO)
  // per product.
  std::vector<GLuint> vertices_vbos;
  std::vector<GLuint> elements_vbos;

  size_t vbo_count = products.products.size();
  if (vbo_count) {
    vertices_vbos.resize(vbo_count);
    // Will default to zeroes, so we don't have to keep checking for the
    // Indexing feature
    elements_vbos.resize(vbo_count);
    glGenBuffers(vbo_count, vertices_vbos.data());
    all_vbos_.insert(all_vbos_.end(), vertices_vbos.begin(), vertices_vbos.end());
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      glGenBuffers(vbo_count, elements_vbos.data());
      all_vbos_.insert(all_vbos_.end(), elements_vbos.begin(), elements_vbos.end());
    }
  }

#ifdef ENABLE_OPENCSG
  size_t vbo_index = 0;
  for (auto i = 0; i < products.products.size(); ++i) {
    const auto &product = products.products[i];
    const auto vertices_vbo = vertices_vbos[i];
    const auto elements_vbo = elements_vbos[i];

    Color4f last_color;
    std::vector<OpenCSG::Primitive *> primitives;
    std::unique_ptr<std::vector<std::shared_ptr<VertexState>>> vertex_states =
        std::make_unique<std::vector<std::shared_ptr<VertexState>>>();
    VertexArray vertex_array(std::make_unique<OpenCSGVertexStateFactory>(),
                             *(vertex_states.get()), vertices_vbo,
                             elements_vbo);
    vertex_array.addSurfaceData();
    vertex_array.writeSurface();
    if (getShader().progid != 0) {
      add_shader_data(vertex_array);
    } else {
      LOG("Warning: Shader not available");
    }

    size_t num_vertices = 0;
    for (const auto &csgobj : product.intersections) {
      if (csgobj.leaf->polyset) {
        num_vertices += getSurfaceBufferSize(csgobj);
      }
    }
    for (const auto &csgobj : product.subtractions) {
      if (csgobj.leaf->polyset) {
        num_vertices += getSurfaceBufferSize(csgobj);
      }
    }

    vertex_array.allocateBuffers(num_vertices);

    for (const auto &csgobj : product.intersections) {
      if (csgobj.leaf->polyset) {
        const Color4f &c = csgobj.leaf->color;
        csgmode_e csgmode = get_csgmode(highlight_mode, background_mode);

        ColorMode colormode = ColorMode::NONE;
        bool override_color;
        if (highlight_mode) {
          colormode = ColorMode::HIGHLIGHT;
          override_color = true;
        } else if (background_mode) {
          colormode = ColorMode::BACKGROUND;
          override_color = true;
        } else {
          colormode = ColorMode::MATERIAL;
          override_color = c.isValid();
        }

        Color4f color;
        if (getShaderColor(colormode, c, color)) {
          last_color = color;
        }

        add_color(vertex_array, last_color);

        if (color[3] == 1.0f) {
          // object is opaque, draw normally
          create_surface(*csgobj.leaf->polyset, vertex_array, csgmode,
                         csgobj.leaf->matrix, last_color, override_color);
          std::shared_ptr<OpenCSGVertexState> surface =
              std::dynamic_pointer_cast<OpenCSGVertexState>(
                  vertex_states->back());
          if (surface != nullptr) {
            surface->setCsgObjectIndex(csgobj.leaf->index);
            primitives.emplace_back(
                createVBOPrimitive(surface, OpenCSG::Intersection,
                                   csgobj.leaf->polyset->getConvexity()));
          }
        } else {
          // object is transparent, so draw rear faces first.  Issue #1496
          std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
          cull->glBegin().emplace_back([]() {
            GL_TRACE0("glEnable(GL_CULL_FACE)");
            glEnable(GL_CULL_FACE);
          });
          cull->glBegin().emplace_back([]() {
            GL_TRACE0("glCullFace(GL_FRONT)");
            glCullFace(GL_FRONT);
          });
          vertex_states->emplace_back(std::move(cull));

          create_surface(*csgobj.leaf->polyset, vertex_array, csgmode,
                         csgobj.leaf->matrix, last_color, override_color);
          std::shared_ptr<OpenCSGVertexState> surface =
              std::dynamic_pointer_cast<OpenCSGVertexState>(
                  vertex_states->back());

          if (surface != nullptr) {
            surface->setCsgObjectIndex(csgobj.leaf->index);

            primitives.emplace_back(
                createVBOPrimitive(surface, OpenCSG::Intersection,
                                   csgobj.leaf->polyset->getConvexity()));

            cull = std::make_shared<VertexState>();
            cull->glBegin().emplace_back([]() {
              GL_TRACE0("glCullFace(GL_BACK)");
              glCullFace(GL_BACK);
            });
            vertex_states->emplace_back(std::move(cull));

            vertex_states->emplace_back(surface);

            cull = std::make_shared<VertexState>();
            cull->glEnd().emplace_back([]() {
              GL_TRACE0("glDisable(GL_CULL_FACE)");
              glDisable(GL_CULL_FACE);
            });
            vertex_states->emplace_back(std::move(cull));
          } else {
            assert(false && "Intersection surface state was nullptr");
          }
        }
      }
    }

    for (const auto &csgobj : product.subtractions) {
      if (csgobj.leaf->polyset) {
        const Color4f &c = csgobj.leaf->color;
        csgmode_e csgmode = get_csgmode(highlight_mode, background_mode,
                                        OpenSCADOperator::DIFFERENCE);

        ColorMode colormode = ColorMode::NONE;
        bool override_color;
        if (highlight_mode) {
          colormode = ColorMode::HIGHLIGHT;
          override_color = true;
        } else if (background_mode) {
          colormode = ColorMode::BACKGROUND;
          override_color = true;
        } else {
          colormode = ColorMode::CUTOUT;
          override_color = true;
        }

        Color4f color;
        if (getShaderColor(colormode, c, color)) {
          last_color = color;
        }

        add_color(vertex_array, last_color);

        // negative objects should only render rear faces
        std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
        cull->glBegin().emplace_back([]() {
          GL_TRACE0("glEnable(GL_CULL_FACE)");
          GL_CHECKD(glEnable(GL_CULL_FACE));
        });
        cull->glBegin().emplace_back([]() {
          GL_TRACE0("glCullFace(GL_FRONT)");
          GL_CHECKD(glCullFace(GL_FRONT));
        });
        vertex_states->emplace_back(std::move(cull));
        Transform3d tmp = csgobj.leaf->matrix;
        if (csgobj.leaf->polyset->getDimension() == 2) {
          // Scale 2D negative objects 10% in the Z direction to avoid z fighting
          tmp *= Eigen::Scaling(1.0, 1.0, 1.1);
        }
        create_surface(*csgobj.leaf->polyset, vertex_array, csgmode, tmp,
                       last_color, override_color);
        std::shared_ptr<OpenCSGVertexState> surface =
            std::dynamic_pointer_cast<OpenCSGVertexState>(
                vertex_states->back());
        if (surface != nullptr) {
          surface->setCsgObjectIndex(csgobj.leaf->index);
          primitives.emplace_back(
              createVBOPrimitive(surface, OpenCSG::Subtraction,
                                 csgobj.leaf->polyset->getConvexity()));
        } else {
          assert(false && "Subtraction surface state was nullptr");
        }

        cull = std::make_shared<VertexState>();
        cull->glEnd().emplace_back([]() {
          GL_TRACE0("glDisable(GL_CULL_FACE)");
          GL_CHECKD(glDisable(GL_CULL_FACE));
        });
        vertex_states->emplace_back(std::move(cull));
      }
    }

    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));

    vertex_array.createInterleavedVBOs();
    vbo_vertex_products_.emplace_back(std::make_unique<OpenCSGVBOProduct>(
        std::move(primitives), std::move(vertex_states)));
  }
#endif // ENABLE_OPENCSG
}

void OpenCSGRenderer::renderCSGVBOProducts(
    bool showedges, const Renderer::shaderinfo_t *shaderinfo) const {
#ifdef ENABLE_OPENCSG
  for (const auto &product : vbo_vertex_products_) {
    if (product->primitives().size() > 1) {
      GL_CHECKD(OpenCSG::render(product->primitives()));
      GL_TRACE0("glDepthFunc(GL_EQUAL)");
      GL_CHECKD(glDepthFunc(GL_EQUAL));
    }

    if (shaderinfo && shaderinfo->progid) {
      GL_TRACE("glUseProgram(%d)", shaderinfo->progid);
      GL_CHECKD(glUseProgram(shaderinfo->progid));

      if (shaderinfo->type == EDGE_RENDERING && showedges) {
        shader_attribs_enable();
      }
    }

    for (const auto &vs : product->states()) {
      if (vs) {
        std::shared_ptr<OpenCSGVertexState> csg_vs =
            std::dynamic_pointer_cast<OpenCSGVertexState>(vs);
        if (csg_vs) {
          if (shaderinfo && shaderinfo->type == SELECT_RENDERING) {
            GL_TRACE("glUniform3f(%d, %f, %f, %f)",
                     shaderinfo->data.select_rendering.identifier %
                         (((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f) %
                         (((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f) %
                         (((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
            GL_CHECKD(glUniform3f(
                shaderinfo->data.select_rendering.identifier,
                ((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f,
                ((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f,
                ((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
          }
        }
        std::shared_ptr<VBOShaderVertexState> shader_vs =
            std::dynamic_pointer_cast<VBOShaderVertexState>(vs);
        if (!shader_vs || (showedges && shader_vs)) {
          vs->draw();
        }
      }
    }

    if (shaderinfo && shaderinfo->progid) {
      GL_TRACE0("glUseProgram(0)");
      GL_CHECKD(glUseProgram(0));

      if (shaderinfo->type == EDGE_RENDERING && showedges) {
        shader_attribs_disable();
      }
    }
    GL_TRACE0("glDepthFunc(GL_LEQUAL)");
    GL_CHECKD(glDepthFunc(GL_LEQUAL));
  }
#endif // ENABLE_OPENCSG
}

BoundingBox OpenCSGRenderer::getBoundingBox() const {
  BoundingBox bbox;
  if (root_products_)
    bbox = root_products_->getBoundingBox();
  if (highlights_products_)
    bbox.extend(highlights_products_->getBoundingBox());
  if (background_products_)
    bbox.extend(background_products_->getBoundingBox());

  return bbox;
}
