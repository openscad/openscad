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

#include "PolySetRenderer.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>
#include <memory>
#include <vector>
#ifdef _MSC_VER
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

#include "glview/system-gl.h"
#include "core/Selection.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "glview/ColorMap.h"
#include "glview/VBORenderer.h"
#include "glview/Renderer.h"
#include "glview/ShaderUtils.h"
#include "glview/VBOBuilder.h"
#include "glview/VertexState.h"
#include "utils/vector_math.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALNefGeometry.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

// This renderer is used in Manifold mode (F6 with Manifold as geometry engine)
PolySetRenderer::PolySetRenderer(const std::shared_ptr<const class Geometry>& geom)
{
  this->addGeometry(geom);
}

void PolySetRenderer::addGeometry(const std::shared_ptr<const Geometry>& geom)
{
  assert(geom != nullptr);
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      this->addGeometry(item.second);
    }
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(ps->getDimension() == 3);
    // We need to tessellate here, in case the generated PolySet contains concave polygons
    // See tests/data/scad/3D/features/polyhedron-concave-test.scad
    this->polysets_.push_back(PolySetUtils::tessellate_faces(*ps));
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    this->polygons_.emplace_back(poly, std::shared_ptr<const PolySet>(poly->tessellate()));
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    this->polysets_.push_back(mani->toPolySet());
#endif
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGALNefGeometry>(geom)) {
    // Note: It's rare, but possible for Nef polyhedrons to exist among geometries in Manifold mode.
    // One way is through import("file.nef3")
    assert(N->getDimension() == 3);
    if (!N->isEmpty()) {
      if (auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*N->p3)) {
        ps->setConvexity(N->getConvexity());
        this->polysets_.push_back(std::shared_ptr<PolySet>(std::move(ps)));
      }
    }
#endif
  } else {
    const auto& geom_ref = *geom.get();
    LOG("Unsupported geom '%1$s' in PolySetRenderer", typeid(geom_ref).name());
    assert(false && "Unsupported geom in PolySetRenderer");
  }
}

// Overridden from Renderer
void PolySetRenderer::setColorScheme(const ColorScheme& cs)
{
  Renderer::setColorScheme(cs);
  colormap_[ColorMode::CGAL_FACE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_FACE_2D_COLOR);
  colormap_[ColorMode::CGAL_EDGE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_2D_COLOR);
}

void PolySetRenderer::createPolySetStates(const ShaderUtils::ShaderInfo *shaderinfo)
{
  VertexStateContainer& vertex_state_container = polyset_vertex_state_containers_.emplace_back();
  VBOBuilder vbo_builder(std::make_unique<VertexStateFactory>(), vertex_state_container);

  vbo_builder.addSurfaceData();  // position, normal, color
  vbo_builder.addShaderData();
  const bool enable_barycentric = true;

  size_t num_vertices = 0;
  for (const auto& polyset : this->polysets_) {
    num_vertices += calcNumVertices(*polyset);
  }
  vbo_builder.allocateBuffers(num_vertices);

  for (const auto& polyset : this->polysets_) {
    Color4f color;
    if (!polyset->colors.empty()) color = polyset->colors[0];
    getShaderColor(ColorMode::MATERIAL, color, color);
    add_shader_pointers(vbo_builder, shaderinfo);

    vbo_builder.writeSurface();
    vbo_builder.create_surface(*polyset, Transform3d::Identity(), color, enable_barycentric, false);
  }

  vbo_builder.createInterleavedVBOs();
}

void PolySetRenderer::createPolygonStates()
{
  createPolygonSurfaceStates();
  createPolygonEdgeStates();
}

void PolySetRenderer::createPolygonSurfaceStates()
{
  VertexStateContainer& vertex_state_container = polygon_vertex_state_containers_.emplace_back();
  VBOBuilder vbo_builder(std::make_unique<VertexStateFactory>(), vertex_state_container);
  vbo_builder.addSurfaceData();

  size_t num_vertices = 0;
  for (const auto& [_, polyset] : this->polygons_) {
    num_vertices += calcNumVertices(*polyset);
  }

  vbo_builder.allocateBuffers(num_vertices);

  std::shared_ptr<VertexState> init_state = std::make_shared<VertexState>();
  init_state->glBegin().emplace_back([]() {
    GL_TRACE0("glDisable(GL_LIGHTING)");
    GL_CHECKD(glDisable(GL_LIGHTING));
  });
  vertex_state_container.states().emplace_back(std::move(init_state));

  for (const auto& [polygon, polyset] : this->polygons_) {
    Color4f color;
    getColorSchemeColor(ColorMode::CGAL_FACE_2D_COLOR, color);
    vbo_builder.create_polygons(*polyset, Transform3d::Identity(), color);
  }

  vbo_builder.createInterleavedVBOs();
}

void PolySetRenderer::createPolygonEdgeStates()
{
  VertexStateContainer& vertex_state_container = polygon_vertex_state_containers_.emplace_back();
  VBOBuilder vbo_builder(std::make_unique<VertexStateFactory>(), vertex_state_container);

  vbo_builder.addEdgeData();

  size_t num_vertices = 0;
  for (const auto& [polygon, _] : this->polygons_) {
    num_vertices += calcNumEdgeVertices(*polygon);
  }

  vbo_builder.allocateBuffers(num_vertices);

  std::shared_ptr<VertexState> edge_state = std::make_shared<VertexState>();
  edge_state->glBegin().emplace_back([]() {
    GL_TRACE0("glDisable(GL_DEPTH_TEST)");
    GL_CHECKD(glDisable(GL_DEPTH_TEST));
    GL_TRACE0("glLineWidth(2)");
    GL_CHECKD(glLineWidth(2));
  });
  vertex_state_container.states().emplace_back(std::move(edge_state));

  for (const auto& [polygon, _] : this->polygons_) {
    Color4f color;
    getColorSchemeColor(ColorMode::CGAL_EDGE_2D_COLOR, color);
    vbo_builder.writeEdge();
    vbo_builder.create_edges(*polygon, Transform3d::Identity(), color);
  }

  std::shared_ptr<VertexState> end_state = std::make_shared<VertexState>();
  end_state->glBegin().emplace_back([]() {
    GL_TRACE0("glEnable(GL_DEPTH_TEST)");
    GL_CHECKD(glEnable(GL_DEPTH_TEST));
  });
  vertex_state_container.states().emplace_back(std::move(end_state));

  vbo_builder.createInterleavedVBOs();
}

void PolySetRenderer::prepare(const ShaderUtils::ShaderInfo *shaderinfo)
{
  if (polyset_vertex_state_containers_.empty() && polygon_vertex_state_containers_.empty()) {
    if (!this->polysets_.empty() && !this->polygons_.empty()) {
      LOG(message_group::Error, "PolySetRenderer::prepare() called with both polysets and polygons");
    } else if (!this->polysets_.empty()) {
      createPolySetStates(shaderinfo);
    } else if (!this->polygons_.empty()) {
      createPolygonStates();
    }
  }
}

void PolySetRenderer::draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo) const
{
  drawPolySets(showedges, shaderinfo);
  drawPolygons();
}

void PolySetRenderer::drawPolySets(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo) const
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

  for (const auto& container : polyset_vertex_state_containers_) {
    for (const auto& vertex_state : container.states()) {
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

void PolySetRenderer::drawPolygons() const
{
  // grab current state to restore after
  GLfloat current_point_size, current_line_width;
  const GLboolean origVBOBuilderState = glIsEnabled(GL_VERTEX_ARRAY);
  const GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
  const GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

  GL_CHECKD(glGetFloatv(GL_POINT_SIZE, &current_point_size));
  GL_CHECKD(glGetFloatv(GL_LINE_WIDTH, &current_line_width));

  for (const auto& container : polygon_vertex_state_containers_) {
    for (const auto& vertex_state : container.states()) {
      if (vertex_state) vertex_state->draw();
    }
  }

  // restore states
  GL_TRACE("glPointSize(%d)", current_point_size);
  GL_CHECKD(glPointSize(current_point_size));
  GL_TRACE("glLineWidth(%d)", current_line_width);
  GL_CHECKD(glLineWidth(current_line_width));

  if (!origVBOBuilderState) glDisableClientState(GL_VERTEX_ARRAY);
  if (!origNormalArrayState) glDisableClientState(GL_NORMAL_ARRAY);
  if (!origColorArrayState) glDisableClientState(GL_COLOR_ARRAY);
}

BoundingBox PolySetRenderer::getBoundingBox() const
{
  BoundingBox bbox;

  for (const auto& ps : this->polysets_) {
    bbox.extend(ps->getBoundingBox());
  }
  for (const auto& [polygon, polyset] : this->polygons_) {
    bbox.extend(polygon->getBoundingBox());
  }
  return bbox;
}

std::vector<SelectedObject> PolySetRenderer::findModelObject(const Vector3d& near_pt,
                                                             const Vector3d& far_pt, int /*mouse_x*/,
                                                             int /*mouse_y*/, double tolerance)
{
  std::vector<SelectedObject> results;
  double dist_nearest = NAN;
  Vector3d pt1_nearest;
  Vector3d pt2_nearest;

  // This only considers vertices near the line containing near_pt and far_pt, and chooses either the
  // first one it iterates past which is on the near side of `near_pt` (due to clamping of dist_near) or
  // the one with the closest tangent intersection to `near_pt`, if none are on the near side.
  for (const auto& ps : this->polysets_) {
    for (const auto& pt : ps->vertices) {
      double dist_near;
      const double dist_pt = calculateLinePointDistance(near_pt, far_pt, pt, dist_near);
      if (dist_pt < tolerance) {
        if (isnan(dist_nearest) || dist_near < dist_nearest) {
          dist_nearest = dist_near;
          pt1_nearest = pt;
        }
      }
    }
  }
  if (!isnan(dist_nearest)) {
    // We found an acceptable vertex.
    const SelectedObject obj = {
      .type = SelectionType::SELECTION_POINT,
      .p1 = pt1_nearest,
    };
    results.push_back(obj);
    return results;
  }
  for (const std::shared_ptr<const PolySet>& ps : this->polysets_) {
    for (const auto& pol : ps->indices) {
      const int n = pol.size();
      for (int i = 0; i < n; i++) {
        const int ind1 = pol[i];
        const int ind2 = pol[(i + 1) % n];
        double parametric_t;
        const double dist_norm = std::fabs(calculateLineLineDistance(
          ps->vertices[ind1], ps->vertices[ind2], near_pt, far_pt, parametric_t));
        if (parametric_t >= 0 && parametric_t <= 1 && dist_norm < tolerance) {
          // The closest point falls on the line segment from
          // ps->vertices[ind1] to ps->vertices[ind2],
          // and it's less than tolerance away.
          dist_nearest = parametric_t;
          pt1_nearest = ps->vertices[ind1];
          pt2_nearest = ps->vertices[ind2];
          // I don't know why we don't break out of the top for loop.
          // Is there anything special about picking the last answer instead of the first?
        }
      }
    }
  }

  if (!isnan(dist_nearest)) {
    // We found an acceptable line segment.
    const SelectedObject obj = {
      .type = SelectionType::SELECTION_LINE,
      .p1 = pt1_nearest,
      .p2 = pt2_nearest,
    };
    results.push_back(obj);
    return results;
  }
  return results;
}
