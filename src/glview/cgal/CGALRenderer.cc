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
#include <limits>
#include <utility>
#include <memory>

#ifdef _MSC_VER
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

#include "Feature.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"

#include "glview/cgal/CGALRenderUtils.h"
#ifdef ENABLE_CGAL
#include "glview/cgal/VBOPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include <cstddef>
#include <vector>

// #include "gui/Preferences.h"

CGALRenderer::CGALRenderer(const std::shared_ptr<const class Geometry> &geom) {
  this->addGeometry(geom);
  PRINTD("CGALRenderer::CGALRenderer() -> createPolyhedrons()");
#ifdef ENABLE_CGAL
  if (!this->nefPolyhedrons_.empty() && this->polyhedrons_.empty())
    createPolyhedrons();
#endif
}

void CGALRenderer::addGeometry(const std::shared_ptr<const Geometry> &geom) {
  if (const auto geomlist =
          std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto &item : geomlist->getChildren()) {
      this->addGeometry(item.second);
    }
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(ps->getDimension() == 3);
    // We need to tessellate here, in case the generated PolySet contains
    // concave polygons See
    // tests/data/scad/3D/features/polyhedron-concave-test.scad
    this->polysets_.push_back(PolySetUtils::tessellate_faces(*ps));
  } else if (const auto poly =
                 std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    this->polygons_.emplace_back(
        poly, std::shared_ptr<const PolySet>(poly->tessellate()));
#ifdef ENABLE_CGAL
  } else if (const auto new_N =
                 std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    assert(new_N->getDimension() == 3);
    if (!new_N->isEmpty()) {
      this->nefPolyhedrons_.push_back(new_N);
    }
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani =
                 std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    this->polysets_.push_back(mani->toPolySet());
#endif
  } else {
    assert(false && "unsupported geom in CGALRenderer");
  }
}

CGALRenderer::~CGALRenderer() {
  if (polyset_vertices_vbo_) {
    glDeleteBuffers(1, &polyset_vertices_vbo_);
  }
  if (polyset_elements_vbo_) {
    glDeleteBuffers(1, &polyset_elements_vbo_);
  }
}

#ifdef ENABLE_CGAL
void CGALRenderer::createPolyhedrons() {
  PRINTD("createPolyhedrons");
  this->polyhedrons_.clear();
  for (const auto &N : this->nefPolyhedrons_) {
    auto p = std::make_shared<VBOPolyhedron>(*colorscheme_);
    CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(
        *N->p3, p.get());
    // CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
    // CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
    p->init();
    this->polyhedrons_.push_back(p);
  }
  PRINTD("createPolyhedrons() end");
}
#endif

// Overridden from Renderer
void CGALRenderer::setColorScheme(const ColorScheme &cs) {
  PRINTD("setColorScheme");
  Renderer::setColorScheme(cs);
  colormap_[ColorMode::CGAL_FACE_2D_COLOR] =
      ColorMap::getColor(cs, RenderColor::CGAL_FACE_2D_COLOR);
  colormap_[ColorMode::CGAL_EDGE_2D_COLOR] =
      ColorMap::getColor(cs, RenderColor::CGAL_EDGE_2D_COLOR);
#ifdef ENABLE_CGAL
  this->polyhedrons_.clear(); // Mark as dirty
#endif
  this->vertex_states_.clear(); // Mark as dirty
  PRINTD("setColorScheme done");
}

void CGALRenderer::createPolySetStates() {
  PRINTD("createPolySetStates() polyset");

  vertex_states_.clear();

  glGenBuffers(1, &polyset_vertices_vbo_);
  if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
    glGenBuffers(1, &polyset_elements_vbo_);
  }

  VBOBuilder vertex_array(std::make_unique<VertexStateFactory>(),
                           vertex_states_, polyset_vertices_vbo_,
                           polyset_elements_vbo_);

  vertex_array.addEdgeData();
  vertex_array.addSurfaceData();

  size_t num_vertices = 0;
  for (const auto &polyset : this->polysets_) {
    num_vertices += getSurfaceBufferSize(*polyset);
    num_vertices += getEdgeBufferSize(*polyset);
  }
  for (const auto &[polygon, polyset] : this->polygons_) {
    num_vertices += getSurfaceBufferSize(*polyset);
    num_vertices += getEdgeBufferSize(*polygon);
  }

  vertex_array.allocateBuffers(num_vertices);

  for (const auto &polyset : this->polysets_) {
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

void CGALRenderer::prepare(bool /*showfaces*/, bool /*showedges*/,
                           const RendererUtils::ShaderInfo * /*shaderinfo*/) {
  PRINTD("prepare()");
  if (!vertex_states_.size())
    createPolySetStates();
#ifdef ENABLE_CGAL
  if (!this->nefPolyhedrons_.empty() && this->polyhedrons_.empty())
    createPolyhedrons();
#endif

  PRINTD("prepare() end");
}

void CGALRenderer::draw(bool showfaces, bool showedges,
                        const RendererUtils::ShaderInfo * /*shaderinfo*/) const {
  PRINTD("draw()");
  // grab current state to restore after
  GLfloat current_point_size, current_line_width;
  GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
  GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
  GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

  GL_CHECKD(glGetFloatv(GL_POINT_SIZE, &current_point_size));
  GL_CHECKD(glGetFloatv(GL_LINE_WIDTH, &current_line_width));

  for (const auto &vertex_state : vertex_states_) {
    if (vertex_state)
      vertex_state->draw();
  }

  // restore states
  GL_TRACE("glPointSize(%d)", current_point_size);
  GL_CHECKD(glPointSize(current_point_size));
  GL_TRACE("glLineWidth(%d)", current_line_width);
  GL_CHECKD(glLineWidth(current_line_width));

  if (!origVertexArrayState)
    glDisableClientState(GL_VERTEX_ARRAY);
  if (!origNormalArrayState)
    glDisableClientState(GL_NORMAL_ARRAY);
  if (!origColorArrayState)
    glDisableClientState(GL_COLOR_ARRAY);

#ifdef ENABLE_CGAL
  for (const auto &p : this->getPolyhedrons()) {
    p->draw(showfaces && showedges);
  }
#endif

  PRINTD("draw() end");
}

BoundingBox CGALRenderer::getBoundingBox() const {
  BoundingBox bbox;

#ifdef ENABLE_CGAL
  for (const auto &p : this->getPolyhedrons()) {
    CGAL::Bbox_3 cgalbbox = p->bbox();
    bbox.extend(BoundingBox(
        Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
        Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax())));
  }
#endif
  for (const auto &ps : this->polysets_) {
    bbox.extend(ps->getBoundingBox());
  }
  for (const auto &[polygon, polyset] : this->polygons_) {
    bbox.extend(polygon->getBoundingBox());
  }
  return bbox;
}

std::vector<SelectedObject>
CGALRenderer::findModelObject(Vector3d near_pt, Vector3d far_pt, int mouse_x,
                              int mouse_y, double tolerance) {
  double dist_near;
  double dist_nearest = std::numeric_limits<double>::max();
  Vector3d pt1_nearest;
  Vector3d pt2_nearest;
  const auto find_nearest_point = [&](const std::vector<Vector3d> &vertices){
    for (const Vector3d &pt : vertices) {
      const double dist_pt =
          calculateLinePointDistance(near_pt, far_pt, pt, dist_near);
      if (dist_pt < tolerance && dist_near < dist_nearest) {
        dist_nearest = dist_near;
        pt1_nearest = pt;
      }
    }
  };
  for (const std::shared_ptr<const PolySet> &ps : this->polysets_) {
    find_nearest_point(ps->vertices);
  }
  for (const auto &[polygon, ps] : this->polygons_) {
    find_nearest_point(ps->vertices);
  }
  if (dist_nearest < std::numeric_limits<double>::max()) {
    SelectedObject obj = {
      .type = SelectionType::SELECTION_POINT,
      .p1 = pt1_nearest
    };
    return std::vector<SelectedObject>{obj};
  }

  const auto find_nearest_line = [&](const std::vector<Vector3d> &vertices, const PolygonIndices& indices) {
    for (const auto &poly : indices) {
      for (int i = 0; i < poly.size(); i++) {
        int ind1 = poly[i];
        int ind2 = poly[(i + 1) % poly.size()];
        double dist_lat;
        double dist_norm = fabs(calculateLineLineDistance(
            vertices[ind1], vertices[ind2], near_pt, far_pt, dist_lat));
        if (dist_lat >= 0 && dist_lat <= 1 && dist_norm < tolerance) {
          dist_nearest = dist_lat;
          pt1_nearest = vertices[ind1];
          pt2_nearest = vertices[ind2];
        }
      }
    }
  };
  for (const std::shared_ptr<const PolySet> &ps : this->polysets_) {
    find_nearest_line(ps->vertices, ps->indices);
  }
  for (const auto &[polygon, ps] : this->polygons_) {
    find_nearest_line(ps->vertices, ps->indices);
  }
  if (dist_nearest < std::numeric_limits<double>::max()) {
    SelectedObject obj = {
      .type = SelectionType::SELECTION_LINE,
      .p1 = pt1_nearest,
      .p2 = pt2_nearest,
    };
    return std::vector<SelectedObject>{obj};
  }
  return {};
}
