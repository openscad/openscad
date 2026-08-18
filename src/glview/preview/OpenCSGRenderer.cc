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
#include "glview/Renderer.h"
#include "glview/ShaderUtils.h"
#include "glview/VertexState.h"
#include "geometry/linalg.h"
#include "glview/system-gl.h"

#include "Feature.h"
#include "utils/printutils.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <memory.h>
#include <cstddef>
#include <utility>
#include <vector>
#include <chrono>

#ifdef ENABLE_OPENCSG

namespace {

// ponytail: accumulate elapsed time into a duration for the prepare() phase measurement.
struct ScopedAccum {
  std::chrono::steady_clock::duration& d;
  const std::chrono::steady_clock::time_point t0{std::chrono::steady_clock::now()};
  ~ScopedAccum() { d += std::chrono::steady_clock::now() - t0; }
};

class OpenCSGVBOPrim : public OpenCSG::Primitive
{
public:
  OpenCSGVBOPrim(OpenCSG::Operation operation, unsigned int convexity,
                 std::unique_ptr<VertexState> vertex_state)
    : OpenCSG::Primitive(operation, convexity), vertex_state(std::move(vertex_state))
  {
  }
  void render() override
  {
    if (vertex_state != nullptr) {
      vertex_state->draw();
    } else {
      if (OpenSCAD::debug != "") PRINTD("OpenCSGVBOPrim vertex_state was null");
    }
  }

private:
  const std::unique_ptr<VertexState> vertex_state;
};

// Primitive for drawing using OpenCSG
// Makes a copy of the given VertexState enabling just unlit/uncolored vertex
// rendering
std::unique_ptr<OpenCSGVBOPrim> createVBOPrimitive(
  const std::shared_ptr<OpenCSGVertexState>& vertex_state, const OpenCSG::Operation operation,
  const unsigned int convexity)
{
  std::unique_ptr<VertexState> opencsg_vs = std::make_unique<VertexState>(
    vertex_state->drawMode(), vertex_state->drawSize(), vertex_state->drawType(),
    vertex_state->drawOffset(), vertex_state->elementOffset(), vertex_state->verticesVBO(),
    vertex_state->elementsVBO());
  // First two glBegin entries are the vertex position calls
  opencsg_vs->glBegin().insert(opencsg_vs->glBegin().begin(), vertex_state->glBegin().begin(),
                               vertex_state->glBegin().begin() + 2);
  // First glEnd entry is the disable vertex position call
  opencsg_vs->glEnd().insert(opencsg_vs->glEnd().begin(), vertex_state->glEnd().begin(),
                             vertex_state->glEnd().begin() + 1);

  return std::make_unique<OpenCSGVBOPrim>(operation, convexity, std::move(opencsg_vs));
}

}  // namespace

#endif  // ENABLE_OPENCSG

OpenCSGRenderer::OpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                                 std::shared_ptr<CSGProducts> highlights_products,
                                 std::shared_ptr<CSGProducts> background_products)
  : root_products_(std::move(root_products)),
    highlights_products_(std::move(highlights_products)),
    background_products_(std::move(background_products))
{
  opencsg_vertex_shader_code_ = ShaderUtils::loadShaderSource("OpenCSG.vert");
}

void OpenCSGRenderer::prepare(const ShaderUtils::ShaderInfo *shaderinfo)
{
  prepare(shaderinfo, []() { return true; });
}

bool OpenCSGRenderer::prepare(const ShaderUtils::ShaderInfo *shaderinfo,
                              const std::function<bool()>& shouldContinue)
{
  PrepTimings timings;
  const auto t0 = std::chrono::steady_clock::now();
  bool ok = true;
  if (vertex_state_containers_.empty()) {
    if (root_products_ &&
        !createCSGVBOProducts(*root_products_, false, false, shaderinfo, shouldContinue, timings)) {
      ok = false;
    }
    if (ok && background_products_ &&
        !createCSGVBOProducts(*background_products_, false, true, shaderinfo, shouldContinue, timings)) {
      ok = false;
    }
    if (ok && highlights_products_ &&
        !createCSGVBOProducts(*highlights_products_, true, false, shaderinfo, shouldContinue, timings)) {
      ok = false;
    }
  }
  // ponytail: measurement only -- see PrepTimings in the header.
  const auto ms = [](std::chrono::steady_clock::duration d) {
    return std::chrono::duration<double, std::milli>(d).count();
  };
  // ponytail: stderr too -- the GUI console swallows this when scripting a measurement run.
  fprintf(stderr, "PREPARE renderer=%04x leaves=%zu total=%.1f surface=%.1f continue=%.1f gl=%.1f\n",
          static_cast<unsigned>(reinterpret_cast<uintptr_t>(this) & 0xffff), timings.leaves,
          ms(std::chrono::steady_clock::now() - t0), ms(timings.surface), ms(timings.cont),
          ms(timings.gl));
  LOG(
    "prepare(): total %1$.1f ms = create_surface %2$.1f + shouldContinue %3$.1f + GL upload "
    "%4$.1f (%5$d leaves, renderer #%6$d)",
    ms(std::chrono::steady_clock::now() - t0), ms(timings.surface), ms(timings.cont), ms(timings.gl),
    static_cast<int>(timings.leaves), static_cast<int>(reinterpret_cast<uintptr_t>(this) & 0xffff));
  return ok;
}

void OpenCSGRenderer::draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo) const
{
#ifdef ENABLE_OPENCSG
  // Only use shader if select rendering or showedges
  bool enable_shader =
    shaderinfo && ((shaderinfo->type == ShaderUtils::ShaderType::EDGE_RENDERING && showedges) ||
                   shaderinfo->type == ShaderUtils::ShaderType::SELECT_RENDERING);

  for (const auto& product : vertex_state_containers_) {
    if (product->primitives().size() > 1) {
#if OPENCSG_VERSION >= 0x0180
      if (enable_shader) OpenCSG::setVertexShader(opencsg_vertex_shader_code_);
      else OpenCSG::setVertexShader({});
#endif
      GL_CHECKD(OpenCSG::render(product->primitives()));
      GL_TRACE0("glDepthFunc(GL_EQUAL)");
      GL_CHECKD(glDepthFunc(GL_EQUAL));
    }

    if (enable_shader) {
      GL_TRACE("glUseProgram(%d)", shaderinfo->resource.shader_program);
      GL_CHECKD(glUseProgram(shaderinfo->resource.shader_program));
      VBOUtils::shader_attribs_enable(*shaderinfo);
    }

    for (const auto& vertex_state : product->states()) {
      // Specify ID color if we're using select rendering
      if (shaderinfo && shaderinfo->type == ShaderUtils::ShaderType::SELECT_RENDERING) {
        if (const auto csg_vs = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_state)) {
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
      const auto shader_vs = std::dynamic_pointer_cast<VBOShaderVertexState>(vertex_state);
      if (!shader_vs || (showedges && shader_vs)) {
        vertex_state->draw();
      }
    }

    if (enable_shader) {
      GL_TRACE0("glUseProgram(0)");
      GL_CHECKD(glUseProgram(0));
      VBOUtils::shader_attribs_disable(*shaderinfo);
    }
    GL_TRACE0("glDepthFunc(GL_LEQUAL)");
    GL_CHECKD(glDepthFunc(GL_LEQUAL));
  }
#endif  // ENABLE_OPENCSG
}

// Turn the CSGProducts into VBOs
// Will create one (temporary) VertexArray and one VBO(+EBO) per product
// The VBO will be utilized to render multiple objects with correct state
// management. In the future, we could use a VBO per primitive and potentially
// reuse VBOs, but that requires some more careful state management.
// Note: This function can be called multiple times for different products.
// Each call will add to vbo_vertex_products_.
bool OpenCSGRenderer::createCSGVBOProducts(const CSGProducts& products, bool highlight_mode,
                                           bool background_mode,
                                           const ShaderUtils::ShaderInfo *shaderinfo,
                                           const std::function<bool()>& shouldContinue,
                                           PrepTimings& timings)
{
#ifdef ENABLE_OPENCSG
  bool enable_barycentric = true;
  for (const auto& product : products.products) {
    {
      ScopedAccum a{timings.cont};
      if (!shouldContinue()) return false;
    }
    std::unique_ptr<OpenCSGVBOProduct> vertex_state_container = std::make_unique<OpenCSGVBOProduct>();

    Color4f last_color;
    auto& vertex_states = vertex_state_container->states();
    VBOBuilder vbo_builder(std::make_unique<OpenCSGVertexStateFactory>(), *vertex_state_container.get());
    vbo_builder.addSurfaceData();
    vbo_builder.writeSurface();
    vbo_builder.addShaderData();  // Always enable barycentric coordinates

    size_t num_vertices = 0;
    for (const auto& csgobj : product.intersections) {
      {
        ScopedAccum a{timings.cont};
        if (!shouldContinue()) return false;
      }
      if (csgobj.leaf->polyset) {
        num_vertices += calcNumVertices(csgobj);
      }
    }
    for (const auto& csgobj : product.subtractions) {
      {
        ScopedAccum a{timings.cont};
        if (!shouldContinue()) return false;
      }
      if (csgobj.leaf->polyset) {
        num_vertices += calcNumVertices(csgobj);
      }
    }

    {
      ScopedAccum a{timings.gl};
      vbo_builder.allocateBuffers(num_vertices);
    }

    for (const auto& csgobj : product.intersections) {
      if (csgobj.leaf->polyset) {
        const Color4f& c = csgobj.leaf->color;

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

        add_shader_pointers(vbo_builder, shaderinfo);

        if (color.a() == 1.0f) {
          // object is opaque, draw normally
          ++timings.leaves;
          {
            ScopedAccum surface_timer{timings.surface};
            vbo_builder.create_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, last_color,
                                       enable_barycentric, override_color);
          }
          if (const auto csg_vs = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_states.back())) {
            csg_vs->setCsgObjectIndex(csgobj.leaf->index);
            vertex_state_container->addPrimitive(
              createVBOPrimitive(csg_vs, OpenCSG::Intersection, csgobj.leaf->polyset->getConvexity()));
          }
        } else {
          // object is transparent, so draw rear faces first.  Issue #1496
          std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
          cull->glBegin().emplace_back([]() {
            GL_TRACE0("glEnable(GL_CULL_FACE)");
            glEnable(GL_CULL_FACE);
            GL_TRACE0("glCullFace(GL_FRONT)");
            glCullFace(GL_FRONT);
          });
          vertex_states.emplace_back(std::move(cull));

          ++timings.leaves;
          {
            ScopedAccum surface_timer{timings.surface};
            vbo_builder.create_surface(*csgobj.leaf->polyset, csgobj.leaf->matrix, last_color,
                                       enable_barycentric, override_color);
          }
          if (const auto csg_vs = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_states.back())) {
            csg_vs->setCsgObjectIndex(csgobj.leaf->index);

            vertex_state_container->addPrimitive(
              createVBOPrimitive(csg_vs, OpenCSG::Intersection, csgobj.leaf->polyset->getConvexity()));

            cull = std::make_shared<VertexState>();
            cull->glBegin().emplace_back([]() {
              GL_TRACE0("glCullFace(GL_BACK)");
              glCullFace(GL_BACK);
            });
            vertex_states.emplace_back(std::move(cull));

            vertex_states.emplace_back(csg_vs);

            cull = std::make_shared<VertexState>();
            cull->glEnd().emplace_back([]() {
              GL_TRACE0("glDisable(GL_CULL_FACE)");
              glDisable(GL_CULL_FACE);
            });
            vertex_states.emplace_back(std::move(cull));
          } else {
            assert(false && "Intersection surface state was nullptr");
          }
        }
      }
    }

    for (const auto& csgobj : product.subtractions) {
      if (csgobj.leaf->polyset) {
        const Color4f& c = csgobj.leaf->color;
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

        add_shader_pointers(vbo_builder, shaderinfo);

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
        vertex_states.emplace_back(std::move(cull));
        Transform3d tmp = csgobj.leaf->matrix;
        if (csgobj.leaf->polyset->getDimension() == 2) {
          // Scale 2D negative objects 10% in the Z direction to avoid z fighting
          tmp *= Eigen::Scaling(1.0, 1.0, 1.1);
        }
        ++timings.leaves;
        {
          ScopedAccum surface_timer{timings.surface};
          vbo_builder.create_surface(*csgobj.leaf->polyset, tmp, last_color, enable_barycentric,
                                     override_color);
        }
        if (const auto csg_vs = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_states.back())) {
          csg_vs->setCsgObjectIndex(csgobj.leaf->index);
          vertex_state_container->addPrimitive(
            createVBOPrimitive(csg_vs, OpenCSG::Subtraction, csgobj.leaf->polyset->getConvexity()));
        } else {
          assert(false && "Subtraction surface state was nullptr");
        }

        cull = std::make_shared<VertexState>();
        cull->glEnd().emplace_back([]() {
          GL_TRACE0("glDisable(GL_CULL_FACE)");
          GL_CHECKD(glDisable(GL_CULL_FACE));
        });
        vertex_states.emplace_back(std::move(cull));
      }
    }

    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));

    {
      ScopedAccum a{timings.gl};
      vbo_builder.createInterleavedVBOs();
    }
    vertex_state_containers_.push_back(std::move(vertex_state_container));
  }
#endif  // ENABLE_OPENCSG
  return true;
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
  BoundingBox bbox;
  if (root_products_) {
    bbox = root_products_->getBoundingBox();
  }
  if (highlights_products_) {
    bbox.extend(highlights_products_->getBoundingBox());
  }
  if (background_products_) {
    bbox.extend(background_products_->getBoundingBox());
  }

  return bbox;
}
