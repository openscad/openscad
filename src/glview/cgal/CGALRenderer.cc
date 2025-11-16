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

#include "glview/cgal/CGALRenderer.h"

#include <cassert>
#include <utility>
#include <memory>
#include <cstddef>
#include <vector>

#ifdef _MSC_VER
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

#include "geometry/cgal/cgal.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "glview/ColorMap.h"
#include "glview/Renderer.h"
#include "glview/ShaderUtils.h"
#include "glview/system-gl.h"
#include "glview/VBOBuilder.h"
#include "glview/VertexState.h"
#include "utils/printutils.h"

#ifdef ENABLE_CGAL
#include <CGAL/Bbox_3.h>
#include "CGAL/OGL_helper.h"
#include "glview/cgal/VBOPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

CGALRenderer::CGALRenderer(const std::shared_ptr<const class Geometry>& geom)
{
  this->addGeometry(geom);
  PRINTD("CGALRenderer::CGALRenderer() -> createPolyhedrons()");
#ifdef ENABLE_CGAL
  if (!this->nefPolyhedrons_.empty() && this->polyhedrons_.empty()) createPolyhedrons();
#endif
}

void CGALRenderer::addGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      this->addGeometry(item.second);
    }
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(ps->getDimension() == 3);
    // We need to tessellate here, in case the generated PolySet contains
    // concave polygons See
    // tests/data/scad/3D/features/polyhedron-concave-test.scad
    this->polysets_.push_back(PolySetUtils::tessellate_faces(*ps));
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    this->polygons_.emplace_back(poly, std::shared_ptr<const PolySet>(poly->tessellate()));
#ifdef ENABLE_CGAL
  } else if (const auto new_N = std::dynamic_pointer_cast<const CGALNefGeometry>(geom)) {
    assert(new_N->getDimension() == 3);
    if (!new_N->isEmpty()) {
      this->nefPolyhedrons_.push_back(new_N);
    }
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    this->polysets_.push_back(mani->toPolySet());
#endif
  } else {
    assert(false && "unsupported geom in CGALRenderer");
  }
}

CGALRenderer::~CGALRenderer() {}

#ifdef ENABLE_CGAL
void CGALRenderer::createPolyhedrons()
{
  PRINTD("createPolyhedrons");
  this->polyhedrons_.clear();
  for (const auto& N : this->nefPolyhedrons_) {
    auto p = std::make_shared<VBOPolyhedron>(*colorscheme_);
    CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*N->p3, p.get());
    // CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
    // CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
    p->init();
    this->polyhedrons_.push_back(p);
  }
  PRINTD("createPolyhedrons() end");
}
#endif

// Overridden from Renderer
void CGALRenderer::setColorScheme(const ColorScheme& cs)
{
  PRINTD("setColorScheme");
  Renderer::setColorScheme(cs);
  colormap_[ColorMode::CGAL_FACE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_FACE_2D_COLOR);
  colormap_[ColorMode::CGAL_EDGE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_2D_COLOR);
#ifdef ENABLE_CGAL
  this->polyhedrons_.clear();  // Mark as dirty
#endif
  vertex_state_containers_.clear();  // Mark as dirty
  PRINTD("setColorScheme done");
}

void CGALRenderer::createPolySetStates()
{
  PRINTD("createPolySetStates() polyset");

  VertexStateContainer& vertex_state_container = vertex_state_containers_.emplace_back();

  VBOBuilder vbo_builder(std::make_unique<VertexStateFactory>(), vertex_state_container);

  vbo_builder.addSurfaceData();  // position, normal, color

  size_t num_vertices = 0;
  for (const auto& polyset : this->polysets_) {
    num_vertices += calcNumVertices(*polyset);
  }
  vbo_builder.allocateBuffers(num_vertices);

  for (const auto& polyset : this->polysets_) {
    Color4f color;
    getColorSchemeColor(ColorMode::MATERIAL, color);
    vbo_builder.writeSurface();
    vbo_builder.create_surface(*polyset, Transform3d::Identity(), color, false);
  }

  vbo_builder.createInterleavedVBOs();
}

void CGALRenderer::createPolygonStates()
{
  createPolygonSurfaceStates();
  createPolygonEdgeStates();
}

void CGALRenderer::createPolygonSurfaceStates()
{
  VertexStateContainer& vertex_state_container = vertex_state_containers_.emplace_back();
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

void CGALRenderer::createPolygonEdgeStates()
{
  PRINTD("createPolygonStates()");

  VertexStateContainer& vertex_state_container = vertex_state_containers_.emplace_back();
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

void CGALRenderer::prepare(const ShaderUtils::ShaderInfo * /*shaderinfo*/)
{
  PRINTD("prepare()");
  if (!vertex_state_containers_.size()) {
    if (!this->polysets_.empty() && !this->polygons_.empty()) {
      LOG(message_group::Error, "CGALRenderer::prepare() called with both polysets and polygons");
    } else if (!this->polysets_.empty()) {
      createPolySetStates();
    } else if (!this->polygons_.empty()) {
      createPolygonStates();
    }
  }

#ifdef ENABLE_CGAL
  if (!this->nefPolyhedrons_.empty() && this->polyhedrons_.empty()) createPolyhedrons();
#endif

  PRINTD("prepare() end");
}

void CGALRenderer::draw(bool showedges, const ShaderUtils::ShaderInfo * /*shaderinfo*/) const
{
  PRINTD("draw()");
  // grab current state to restore after
  GLfloat current_point_size, current_line_width;
  const GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
  const GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
  const GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

  GL_CHECKD(glGetFloatv(GL_POINT_SIZE, &current_point_size));
  GL_CHECKD(glGetFloatv(GL_LINE_WIDTH, &current_line_width));

  for (const auto& container : vertex_state_containers_) {
    for (const auto& vertex_state : container.states()) {
      if (vertex_state) vertex_state->draw();
    }
  }

  // restore states
  GL_TRACE("glPointSize(%d)", current_point_size);
  GL_CHECKD(glPointSize(current_point_size));
  GL_TRACE("glLineWidth(%d)", current_line_width);
  GL_CHECKD(glLineWidth(current_line_width));

  if (!origVertexArrayState) glDisableClientState(GL_VERTEX_ARRAY);
  if (!origNormalArrayState) glDisableClientState(GL_NORMAL_ARRAY);
  if (!origColorArrayState) glDisableClientState(GL_COLOR_ARRAY);

#ifdef ENABLE_CGAL
  for (const auto& p : this->getPolyhedrons()) {
    p->draw(showedges);
  }
#endif

  PRINTD("draw() end");
}

BoundingBox CGALRenderer::getBoundingBox() const
{
  BoundingBox bbox;

#ifdef ENABLE_CGAL
  for (const auto& p : this->getPolyhedrons()) {
    const CGAL::Bbox_3 cgalbbox = p->bbox();
    bbox.extend(BoundingBox(Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
                            Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax())));
  }
#endif
  for (const auto& ps : this->polysets_) {
    bbox.extend(ps->getBoundingBox());
  }
  for (const auto& [polygon, polyset] : this->polygons_) {
    bbox.extend(polygon->getBoundingBox());
  }
  return bbox;
}
