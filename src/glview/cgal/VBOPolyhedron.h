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

#pragma once

#include <cassert>
#include <utility>
#include <memory>
#include <cstdlib>
#include <vector>

#include <CGAL/IO/Color.h>

#include "CGAL/OGL_helper.h"
#include "glview/ColorMap.h"
#include "glview/VertexState.h"
#include "glview/system-gl.h"
#include "glview/VBOBuilder.h"
#include "glview/ColorMap.h"
#include "utils/printutils.h"

class VBOPolyhedron : public CGAL::OGL::Polyhedron
{
public:
  enum CGALColorIndex {
    MARKED_VERTEX_COLOR = 0,
    MARKED_EDGE_COLOR,
    MARKED_FACET_COLOR,
    UNMARKED_VERTEX_COLOR,
    UNMARKED_EDGE_COLOR,
    UNMARKED_FACET_COLOR,
    NUM_COLORS
  };

  VBOPolyhedron(const ColorScheme& cs)
  {
    // Set default colors.
    setColor(CGALColorIndex::MARKED_VERTEX_COLOR, {0xb7, 0xe8, 0x5c});
    setColor(CGALColorIndex::UNMARKED_VERTEX_COLOR, {0xff, 0xf6, 0x7c});
    setColor(CGALColorIndex::MARKED_FACET_COLOR,
             ColorMap::getColor(cs, RenderColor::CGAL_FACE_BACK_COLOR));
    setColor(CGALColorIndex::UNMARKED_FACET_COLOR,
             ColorMap::getColor(cs, RenderColor::CGAL_FACE_FRONT_COLOR));
    setColor(CGALColorIndex::MARKED_EDGE_COLOR,
             ColorMap::getColor(cs, RenderColor::CGAL_EDGE_BACK_COLOR));
    setColor(CGALColorIndex::UNMARKED_EDGE_COLOR,
             ColorMap::getColor(cs, RenderColor::CGAL_EDGE_FRONT_COLOR));
  }

  ~VBOPolyhedron() override = default;

  void draw(Vertex_iterator v, VBOBuilder& vbo_builder) const
  {
    PRINTD("draw(Vertex_iterator)");

    CGAL::Color c = getVertexColor(v);
    vbo_builder.createVertex({Vector3d(v->x(), v->y(), v->z())}, {},
                             Color4f(c.red(), c.green(), c.blue()), 0, 0, 1);
  }

  void draw(Edge_iterator e, VBOBuilder& vbo_builder) const
  {
    PRINTD("draw(Edge_iterator)");

    const Double_point p = e->source(), q = e->target();
    const CGAL::Color c = getEdgeColor(e);
    const Color4f color(c.red(), c.green(), c.blue());

    vbo_builder.createVertex({Vector3d(p.x(), p.y(), p.z())}, {}, color, 0, 0, true);
    vbo_builder.createVertex({Vector3d(q.x(), q.y(), q.z())}, {}, color, 0, 1, true);
  }

  struct TessUserData {
    GLenum which;
    GLdouble *normal;
    CGAL::Color color;
    size_t primitive_index;
    size_t active_point_index;
    size_t last_size;
    size_t draw_size;
    size_t elements_offset;
    VBOBuilder& vbo_builder;
  };

  static inline void CGAL_GLU_TESS_CALLBACK beginCallback(GLenum which, GLvoid *user)
  {
    auto *tess(static_cast<TessUserData *>(user));
    // Create separate vertex set since "which" could be different draw type
    tess->which = which;
    tess->draw_size = 0;

    tess->last_size = tess->vbo_builder.data()->sizeInBytes();
    tess->elements_offset = 0;
    if (tess->vbo_builder.useElements()) {
      tess->elements_offset = tess->vbo_builder.elements().sizeInBytes();
      // this can vary size if polyset provides triangles
      tess->vbo_builder.addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
      tess->vbo_builder.elementsMap().clear();
    }
  }

  static inline void CGAL_GLU_TESS_CALLBACK endCallback(GLvoid *user)
  {
    auto *tess(static_cast<TessUserData *>(user));

    GLenum elements_type = 0;
    if (tess->vbo_builder.useElements()) elements_type = tess->vbo_builder.elementsData()->glType();
    std::shared_ptr<VertexState> vs =
      tess->vbo_builder.createVertexState(tess->which, tess->draw_size, elements_type,
                                          tess->vbo_builder.writeIndex(), tess->elements_offset);
    tess->vbo_builder.states().emplace_back(std::move(vs));
    tess->vbo_builder.addAttributePointers(tess->last_size);
    tess->primitive_index++;
  }

  static inline void CGAL_GLU_TESS_CALLBACK errorCallback(GLenum errorCode)
  {
    const GLubyte *estring;
    estring = gluErrorString(errorCode);
    fprintf(stderr, "Tessellation Error: %s\n", estring);
    std::exit(0);
  }

  static inline void CGAL_GLU_TESS_CALLBACK vertexCallback(GLvoid *vertex_arg, GLvoid *user_arg)
  {
    auto *vertex(static_cast<GLdouble *>(vertex_arg));
    auto *tess(static_cast<TessUserData *>(user_arg));
    size_t shape_size = 0;

    switch (tess->which) {
    case GL_TRIANGLES:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP: shape_size = 3; break;
    case GL_POINTS:         shape_size = 1; break;
    default:                assert(false && "Unsupported primitive type"); break;
    }

    tess->vbo_builder.createVertex({Vector3d(vertex)}, {Vector3d(tess->normal)},
                                   Color4f(tess->color.red(), tess->color.green(), tess->color.blue()),
                                   0, 0, shape_size);
    tess->draw_size++;
    tess->active_point_index++;
  }

  static inline void CGAL_GLU_TESS_CALLBACK combineCallback(GLdouble coords[3], GLvoid *[4], GLfloat[4],
                                                            GLvoid **dataOut)
  {
    static std::vector<std::unique_ptr<Vector3d>> vertexCache;
    if (dataOut) {
      vertexCache.push_back(std::make_unique<Vector3d>(coords));
      *dataOut = vertexCache.back().get();
    } else {
      vertexCache.clear();
    }
  }

  void draw(Halffacet_iterator f, VBOBuilder& vbo_builder) const
  {
    PRINTD("draw(Halffacet_iterator)");

    GLUtesselator *tess_ = gluNewTess();
    gluTessCallback(tess_, GLenum(GLU_TESS_VERTEX_DATA),
                    (GLvoid(CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) & vertexCallback);
    gluTessCallback(tess_, GLenum(GLU_TESS_COMBINE),
                    (GLvoid(CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) & combineCallback);
    gluTessCallback(tess_, GLenum(GLU_TESS_BEGIN_DATA),
                    (GLvoid(CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) & beginCallback);
    gluTessCallback(tess_, GLenum(GLU_TESS_END_DATA),
                    (GLvoid(CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) & endCallback);
    gluTessCallback(tess_, GLenum(GLU_TESS_ERROR),
                    (GLvoid(CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) & errorCallback);
    gluTessProperty(tess_, GLenum(GLU_TESS_WINDING_RULE), GLU_TESS_WINDING_POSITIVE);

    CGAL::OGL::DFacet::Coord_const_iterator cit;
    TessUserData tess_data = {0, f->normal(), getFacetColor(f), 0, 0, 0, 0, 0, vbo_builder};

    gluTessBeginPolygon(tess_, &tess_data);
    // forall facet cycles of f:
    for (unsigned i = 0; i < f->number_of_facet_cycles(); ++i) {
      gluTessBeginContour(tess_);
      // put all vertices in facet cycle into contour:
      for (cit = f->facet_cycle_begin(i); cit != f->facet_cycle_end(i); ++cit) {
        gluTessVertex(tess_, *cit, *cit);
      }
      gluTessEndContour(tess_);
    }
    gluTessEndPolygon(tess_);
    gluDeleteTess(tess_);
    combineCallback(nullptr, nullptr, nullptr, nullptr);
  }

  void create_polyhedron()
  {
    PRINTD("create_polyhedron");

    points_edges_container_ = std::make_unique<VertexStateContainer>();

    VBOBuilder points_edges_builder(std::make_unique<VertexStateFactory>(),
                                    *points_edges_container_.get());

    points_edges_builder.addEdgeData();
    points_edges_builder.writeEdge();
    size_t last_size = 0;
    size_t elements_offset = 0;

    const size_t num_vertices = vertices_.size() + edges_.size() * 2;
    points_edges_builder.allocateBuffers(num_vertices);

    // Points
    Vertex_iterator v;
    if (points_edges_builder.useElements()) {
      elements_offset = points_edges_builder.elementsOffset();
      points_edges_builder.elementsMap().clear();
    }

    std::shared_ptr<VertexState> settings = std::make_shared<VertexState>();
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glDisable(GL_LIGHTING)");
      GL_CHECKD(glDisable(GL_LIGHTING));
    });
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glPointSize(10.0f)");
      GL_CHECKD(glPointSize(10.0f));
    });
    points_edges_container_->states().emplace_back(std::move(settings));

    for (v = vertices_.begin(); v != vertices_.end(); ++v) draw(v, points_edges_builder);

    GLenum elements_type = 0;
    if (points_edges_builder.useElements())
      elements_type = points_edges_builder.elementsData()->glType();
    std::shared_ptr<VertexState> vs = points_edges_builder.createVertexState(
      GL_POINTS, vertices_.size(), elements_type, points_edges_builder.writeIndex(), elements_offset);
    points_edges_container_->states().emplace_back(std::move(vs));
    points_edges_builder.addAttributePointers(last_size);

    // Edges
    Edge_iterator e;
    last_size = points_edges_builder.verticesOffset();
    elements_offset = 0;
    if (points_edges_builder.useElements()) {
      elements_offset = points_edges_builder.elementsOffset();
      points_edges_builder.elementsMap().clear();
    }

    settings = std::make_shared<VertexState>();
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glDisable(GL_LIGHTING)");
      GL_CHECKD(glDisable(GL_LIGHTING));
    });
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glLineWidth(5.0f)");
      GL_CHECKD(glLineWidth(5.0f));
    });
    points_edges_container_->states().emplace_back(std::move(settings));

    for (e = edges_.begin(); e != edges_.end(); ++e) draw(e, points_edges_builder);

    elements_type = 0;
    if (points_edges_builder.useElements())
      elements_type = points_edges_builder.elementsData()->glType();
    vs = points_edges_builder.createVertexState(GL_LINES, edges_.size() * 2, elements_type,
                                                points_edges_builder.writeIndex(), elements_offset);
    points_edges_container_->states().emplace_back(std::move(vs));
    points_edges_builder.addAttributePointers(last_size);

    points_edges_builder.createInterleavedVBOs();

    // Halffacets
    halffacets_container_ = std::make_unique<VertexStateContainer>();

    // FIXME: We don't know the size of this VertexArray in advanced, so we have to deal with some
    // fallback mechanism for filling in the data. This complicates code quite a bit
    VBOBuilder halffacets_builder(std::make_unique<VertexStateFactory>(), *halffacets_container_.get());
    halffacets_builder.addSurfaceData();
    halffacets_builder.writeSurface();

    settings = std::make_shared<VertexState>();
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glEnable(GL_LIGHTING)");
      GL_CHECKD(glEnable(GL_LIGHTING));
    });
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glLineWidth(5.0f)");
      GL_CHECKD(glLineWidth(5.0f));
    });
    halffacets_container_->states().emplace_back(std::move(settings));

    Halffacet_iterator f;
    for (f = halffacets_.begin(); f != halffacets_.end(); ++f) {
      draw(f, halffacets_builder);
    }

    halffacets_builder.createInterleavedVBOs();
  }

  void init() override
  {
    PRINTD("VBO init()");
    create_polyhedron();
    PRINTD("VBO init() end");
  }

  void draw() const override
  {
    PRINTD("VBO draw()");
    PRINTD("VBO draw() end");
  }

  void draw(bool showedges) const override
  {
    PRINTDB("VBO draw(showedges = %d)", showedges);
    // grab current state to restore after
    GLfloat current_point_size, current_line_width;
    const GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
    const GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
    const GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

    GL_CHECKD(glGetFloatv(GL_POINT_SIZE, &current_point_size));
    GL_CHECKD(glGetFloatv(GL_LINE_WIDTH, &current_line_width));

    for (const auto& halffacet : halffacets_container_->states()) {
      if (halffacet) halffacet->draw();
    }

    if (showedges) {
      for (const auto& point_edge : points_edges_container_->states()) {
        if (point_edge) point_edge->draw();
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

    PRINTD("VBO draw() end");
  }

  // overrides function in OGL_helper.h
  [[nodiscard]] CGAL::Color getVertexColor(Vertex_iterator v) const override
  {
    PRINTD("getVertexColor");
    CGAL::Color c = v->mark() ? colors[CGALColorIndex::UNMARKED_VERTEX_COLOR]
                              : colors[CGALColorIndex::MARKED_VERTEX_COLOR];
    return c;
  }

  // overrides function in OGL_helper.h
  [[nodiscard]] CGAL::Color getEdgeColor(Edge_iterator e) const override
  {
    PRINTD("getEdgeColor");
    CGAL::Color c = e->mark() ? colors[CGALColorIndex::UNMARKED_EDGE_COLOR]
                              : colors[CGALColorIndex::MARKED_EDGE_COLOR];
    return c;
  }

  // overrides function in OGL_helper.h
  [[nodiscard]] CGAL::Color getFacetColor(Halffacet_iterator f) const override
  {
    CGAL::Color c = f->mark() ? colors[CGALColorIndex::UNMARKED_FACET_COLOR]
                              : colors[CGALColorIndex::MARKED_FACET_COLOR];
    return c;
  }

  void setColor(CGALColorIndex color_index, const Color4f& c)
  {
    // Note: Not setting alpha here as none of our built-in colors currently have an alpha component
    // This _may_ yield unexpected results for user-defined colors with alpha components.
    PRINTDB("setColor %i %f %f %f", color_index % c.r() % c.g() % c.b());
    this->colors[color_index] = CGAL::Color(c.r() * 255, c.g() * 255, c.b() * 255);
  }

protected:
  CGAL::Color colors[CGALColorIndex::NUM_COLORS];
  std::unique_ptr<VertexStateContainer> points_edges_container_;
  std::unique_ptr<VertexStateContainer> halffacets_container_;
};  // Polyhedron
