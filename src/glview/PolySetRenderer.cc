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

#ifdef _MSC_VER
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

#include "PolySet.h"
#include "PolySetUtils.h"
#include "printutils.h"
#include "Feature.h"

#include "PolySetRenderer.h"
#include "CGALRenderUtils.h"
#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgalutils.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif

// This renderer is used in Manifold mode (F6 with Manifold as geometry engine)
PolySetRenderer::PolySetRenderer(const std::shared_ptr<const class Geometry>& geom)
{
  this->addGeometry(geom);
}

void PolySetRenderer::addGeometry(const std::shared_ptr<const Geometry>& geom)
{
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
    this->polygons_.emplace_back(
        poly, std::shared_ptr<const PolySet>(poly->tessellate()));
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    this->polysets_.push_back(mani->toPolySet());
#endif
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
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
    LOG("Unsupported geom '%1$s' in PolySetRenderer", typeid(*geom.get()).name());
    assert(false && "Unsupported geom in PolySetRenderer");
  }
}

PolySetRenderer::~PolySetRenderer()
{
  if (vertices_vbo_) {
    glDeleteBuffers(1, &vertices_vbo_);
  }
  if (elements_vbo_) {
    glDeleteBuffers(1, &elements_vbo_);
  }
}

// Overridden from Renderer
void PolySetRenderer::setColorScheme(const ColorScheme& cs)
{
  PRINTD("setColorScheme");
  Renderer::setColorScheme(cs);
  colormap_[ColorMode::CGAL_FACE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_FACE_2D_COLOR);
  colormap_[ColorMode::CGAL_EDGE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_2D_COLOR);
  PRINTD("setColorScheme done");
}

void PolySetRenderer::createVertexStates()
{
  PRINTD("createVertexStates()");

  vertex_states_.clear();

  glGenBuffers(1, &vertices_vbo_);
  if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
    glGenBuffers(1, &elements_vbo_);
  }

  VertexArray vertex_array(std::make_unique<VertexStateFactory>(), 
                           vertex_states_, vertices_vbo_, elements_vbo_);

  vertex_array.addEdgeData();
  vertex_array.addSurfaceData();


  size_t num_vertices = 0;
  for (const auto& polyset : this->polysets_) {
    num_vertices += getSurfaceBufferSize(*polyset);
    num_vertices += getEdgeBufferSize(*polyset);
  }
  for (const auto &[polygon, polyset] : this->polygons_) {
    num_vertices += getSurfaceBufferSize(*polyset);
    num_vertices += getEdgeBufferSize(*polygon);
  }

  vertex_array.allocateBuffers(num_vertices);

  for (const auto& polyset : this->polysets_) {
    Color4f color;

    PRINTD("3d polysets");
    vertex_array.writeSurface();

    // Create 3D polygons
    getColor(ColorMode::MATERIAL, color);
    this->create_surface(*polyset, vertex_array, RendererUtils::CSGMODE_NORMAL,
                         Transform3d::Identity(), color);
  }

  for (const auto &[polygon, polyset] : this->polygons_) {
    Color4f color;

    PRINTD("2d polygons");
    vertex_array.writeEdge();

    std::shared_ptr<VertexState> init_state = std::make_shared<VertexState>();
    init_state->glEnd().emplace_back([]() {
      GL_TRACE0("glDisable(GL_LIGHTING)");
      GL_CHECKD(glDisable(GL_LIGHTING));
    });
    vertex_states_.emplace_back(std::move(init_state));

    // Create 2D polygons
    getColor(ColorMode::CGAL_FACE_2D_COLOR, color);
    this->create_polygons(*polyset, vertex_array, Transform3d::Identity(),
                          color);

    std::shared_ptr<VertexState> edge_state = std::make_shared<VertexState>();
    edge_state->glBegin().emplace_back([]() {
      GL_TRACE0("glDisable(GL_DEPTH_TEST)");
      GL_CHECKD(glDisable(GL_DEPTH_TEST));
    });
    edge_state->glBegin().emplace_back([]() {
      GL_TRACE0("glLineWidth(2)");
      GL_CHECKD(glLineWidth(2));
    });
    vertex_states_.emplace_back(std::move(edge_state));

    // Create 2D edges
    getColor(ColorMode::CGAL_EDGE_2D_COLOR, color);
    this->create_edges(*polygon, vertex_array, Transform3d::Identity(), color);

    std::shared_ptr<VertexState> end_state = std::make_shared<VertexState>();
    end_state->glBegin().emplace_back([]() {
      GL_TRACE0("glEnable(GL_DEPTH_TEST)");
      GL_CHECKD(glEnable(GL_DEPTH_TEST));
    });
    vertex_states_.emplace_back(std::move(end_state));
  }
  
  if (!this->polysets_.empty() || !this->polygons_.empty()) {
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));

    vertex_array.createInterleavedVBOs();
  }
}

void PolySetRenderer::prepare(bool /*showfaces*/, bool /*showedges*/, const RendererUtils::ShaderInfo * /*shaderinfo*/)
{
  PRINTD("prepare()");
  if (!vertex_states_.size()) {
    createVertexStates();
  }
  PRINTD("prepare() end");
}

void PolySetRenderer::draw(bool showfaces, bool showedges, const RendererUtils::ShaderInfo * /*shaderinfo*/) const
{
  PRINTD("draw()");
  // grab current state to restore after
  GLfloat current_point_size, current_line_width;
  const GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
  const GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
  const GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

  GL_CHECKD(glGetFloatv(GL_POINT_SIZE, &current_point_size));
  GL_CHECKD(glGetFloatv(GL_LINE_WIDTH, &current_line_width));

  for (const auto& polyset : vertex_states_) {
    if (polyset) polyset->draw();
  }

  // restore states
  GL_TRACE("glPointSize(%d)", current_point_size);
  GL_CHECKD(glPointSize(current_point_size));
  GL_TRACE("glLineWidth(%d)", current_line_width);
  GL_CHECKD(glLineWidth(current_line_width));

  if (!origVertexArrayState) glDisableClientState(GL_VERTEX_ARRAY);
  if (!origNormalArrayState) glDisableClientState(GL_NORMAL_ARRAY);
  if (!origColorArrayState) glDisableClientState(GL_COLOR_ARRAY);
 
  PRINTD("draw() end");
}

BoundingBox PolySetRenderer::getBoundingBox() const
{
  BoundingBox bbox;

  for (const auto& ps : this->polysets_) {
    bbox.extend(ps->getBoundingBox());
  }
  for (const auto &[polygon, polyset] : this->polygons_) {
    bbox.extend(polygon->getBoundingBox());
  }
  return bbox;
}

std::vector<SelectedObject> PolySetRenderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x, int mouse_y, double tolerance) {
  std::vector<SelectedObject> results;
  double dist_near;
  double dist_nearest = NAN;
  Vector3d pt1_nearest;
  Vector3d pt2_nearest;
  for (const auto& ps : this->polysets_) {
    for (const auto &pt: ps->vertices) {
      const double dist_pt= calculateLinePointDistance(near_pt, far_pt, pt, dist_near);
      if (dist_pt < tolerance) {
        if (isnan(dist_nearest) || dist_near < dist_nearest) {
          dist_nearest = dist_near;
          pt1_nearest = pt;
        }      
      }
    }
  }
  if (!isnan(dist_nearest)) {
    const SelectedObject obj = {
      .type = SelectionType::SELECTION_POINT,
      .p1=pt1_nearest,
    };
    results.push_back(obj);
    return results;
  }
  for (const std::shared_ptr<const PolySet>& ps : this->polysets_) {
    for(const auto &pol : ps->indices) {
      const int n = pol.size();
      for(int i=0;i<n;i++) {
        const int ind1 = pol[i];
        const int ind2 = pol[(i+1)%n];
        double dist_lat;
        const double dist_norm = fabs(calculateLineLineDistance(ps->vertices[ind1], ps->vertices[ind2], near_pt, far_pt,dist_lat));
        if (dist_lat >= 0 && dist_lat <= 1 && dist_norm < tolerance) {
          dist_nearest = dist_lat;
          pt1_nearest = ps->vertices[ind1];
          pt2_nearest = ps->vertices[ind2];
        }
      }      
    }
  }

  if (!isnan(dist_nearest)) {
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
