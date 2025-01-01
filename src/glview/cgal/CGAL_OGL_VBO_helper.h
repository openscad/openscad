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

#include "glview/system-gl.h"
#include "glview/VertexArray.h"
#include "CGAL/OGL_helper.h"

#include <cassert>
#include <utility>
#include <memory>
#include <cstdlib>
#include <vector>

using namespace CGAL::OGL;

// ----------------------------------------------------------------------------
// OGL Drawable Polyhedron:
// ----------------------------------------------------------------------------
class VBOPolyhedron : public virtual Polyhedron
{
public:
  VBOPolyhedron() : Polyhedron() {}
  ~VBOPolyhedron() override
  {
    if (points_edges_vertices_vbo) glDeleteBuffers(1, &points_edges_vertices_vbo);
    if (points_edges_elements_vbo) glDeleteBuffers(1, &points_edges_elements_vbo);
    if (halffacets_vertices_vbo) glDeleteBuffers(1, &halffacets_vertices_vbo);
    if (halffacets_elements_vbo) glDeleteBuffers(1, &halffacets_elements_vbo);
  }

  using CGAL::OGL::Polyhedron::draw;
  void draw(Vertex_iterator v, VertexArray& vertex_array) const {
    PRINTD("draw(Vertex_iterator)");

    CGAL::Color c = getVertexColor(v);
    vertex_array.createVertex({Vector3d(v->x(), v->y(), v->z())},
                              {},
                              Color4f(c.red(), c.green(), c.blue()),
                              0, 0, 1);
  }

  void draw(Edge_iterator e, VertexArray& vertex_array) const {
    PRINTD("draw(Edge_iterator)");

    Double_point p = e->source(), q = e->target();
    CGAL::Color c = getEdgeColor(e);
    Color4f color(c.red(), c.green(), c.blue());

    vertex_array.createVertex({Vector3d(p.x(), p.y(), p.z())},
                              {},
                              color,
                              0, 0, true);
    vertex_array.createVertex({Vector3d(q.x(), q.y(), q.z())},
                              {},
                              color,
                              0, 1, true);
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
    VertexArray& vertex_array;
  };

  static inline void CGAL_GLU_TESS_CALLBACK beginCallback(GLenum which, GLvoid *user) {
    auto *tess(static_cast<TessUserData *>(user));
    // Create separate vertex set since "which" could be different draw type
    tess->which = which;
    tess->draw_size = 0;

    tess->last_size = tess->vertex_array.data()->sizeInBytes();
    tess->elements_offset = 0;
    if (tess->vertex_array.useElements()) {
      tess->elements_offset = tess->vertex_array.elements().sizeInBytes();
      // this can vary size if polyset provides triangles
      tess->vertex_array.addElementsData(std::make_shared<AttributeData<GLuint, 1, GL_UNSIGNED_INT>>());
      tess->vertex_array.elementsMap().clear();
    }
  }

  static inline void CGAL_GLU_TESS_CALLBACK endCallback(GLvoid *user) {
    auto *tess(static_cast<TessUserData *>(user));

    GLenum elements_type = 0;
    if (tess->vertex_array.useElements()) elements_type = tess->vertex_array.elementsData()->glType();
    std::shared_ptr<VertexState> vs = tess->vertex_array.createVertexState(
      tess->which, tess->draw_size, elements_type,
      tess->vertex_array.writeIndex(), tess->elements_offset);
    tess->vertex_array.states().emplace_back(std::move(vs));
    tess->vertex_array.addAttributePointers(tess->last_size);
    tess->primitive_index++;
  }

  static inline void CGAL_GLU_TESS_CALLBACK errorCallback(GLenum errorCode) {
    const GLubyte *estring;
    estring = gluErrorString(errorCode);
    fprintf(stderr, "Tessellation Error: %s\n", estring);
    std::exit(0);
  }

  static inline void CGAL_GLU_TESS_CALLBACK vertexCallback(GLvoid *vertex_arg, GLvoid *user_arg) {
    auto *vertex(static_cast<GLdouble *>(vertex_arg));
    auto *tess(static_cast<TessUserData *>(user_arg));
    size_t shape_size = 0;

    switch (tess->which) {
    case GL_TRIANGLES:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
      shape_size = 3;
      break;
    case GL_POINTS:
      shape_size = 1;
      break;
    default:
      assert(false && "Unsupported primitive type");
      break;
    }


    tess->vertex_array.createVertex({Vector3d(vertex)},
                                    {Vector3d(tess->normal)},
                                    Color4f(tess->color.red(), tess->color.green(), tess->color.blue()),
                                    0, 0, shape_size);
    tess->draw_size++;
    tess->active_point_index++;
  }

  static inline void CGAL_GLU_TESS_CALLBACK combineCallback(GLdouble coords[3], GLvoid *[4], GLfloat [4], GLvoid **dataOut) {
    static std::vector<std::unique_ptr<Vector3d>> vertexCache;
    if (dataOut) {
      vertexCache.push_back(std::make_unique<Vector3d>(coords));
      *dataOut = vertexCache.back().get();
    } else {
      vertexCache.clear();
    }
  }

  void draw(Halffacet_iterator f, VertexArray& vertex_array, bool is_back_facing) const {
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
    gluTessProperty(tess_, GLenum(GLU_TESS_WINDING_RULE),
                    GLU_TESS_WINDING_POSITIVE);

    DFacet::Coord_const_iterator cit;
    TessUserData tess_data = {
      0, f->normal(), getFacetColor(f, is_back_facing),
      0, 0, 0, 0, 0, vertex_array
    };

    gluTessBeginPolygon(tess_, &tess_data);
    // forall facet cycles of f:
    for (unsigned i = 0; i < f->number_of_facet_cycles(); ++i) {
      gluTessBeginContour(tess_);
      // put all vertices in facet cycle into contour:
      for (cit = f->facet_cycle_begin(i);
           cit != f->facet_cycle_end(i); ++cit) {
        gluTessVertex(tess_, *cit, *cit);
      }
      gluTessEndContour(tess_);
    }
    gluTessEndPolygon(tess_);
    gluDeleteTess(tess_);
    combineCallback(nullptr, nullptr, nullptr, nullptr);
  }

  void create_polyhedron() {
    PRINTD("create_polyhedron");

    glGenBuffers(1, &points_edges_vertices_vbo);
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      glGenBuffers(1, &points_edges_elements_vbo);
    }

    VertexArray points_edges_array(std::make_unique<VertexStateFactory>(), points_edges_states, points_edges_vertices_vbo, points_edges_elements_vbo);

    points_edges_array.addEdgeData();
    points_edges_array.writeEdge();
    size_t last_size = 0;
    size_t elements_offset = 0;

    size_t num_vertices = vertices_.size() + edges_.size() * 2, elements_size = 0;
    points_edges_array.allocateBuffers(num_vertices);

    // Points
    Vertex_iterator v;
    if (points_edges_array.useElements()) {
      elements_offset = points_edges_array.elementsOffset();
      points_edges_array.elementsMap().clear();
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
    points_edges_states.emplace_back(std::move(settings));

    for (v = vertices_.begin(); v != vertices_.end(); ++v)
      draw(v, points_edges_array);

    GLenum elements_type = 0;
    if (points_edges_array.useElements()) elements_type = points_edges_array.elementsData()->glType();
    std::shared_ptr<VertexState> vs = points_edges_array.createVertexState(
      GL_POINTS, vertices_.size(), elements_type,
      points_edges_array.writeIndex(), elements_offset);
    points_edges_states.emplace_back(std::move(vs));
    points_edges_array.addAttributePointers(last_size);

    // Edges
    Edge_iterator e;
    last_size = points_edges_array.verticesOffset();
    elements_offset = 0;
    if (points_edges_array.useElements()) {
      elements_offset = points_edges_array.elementsOffset();
      points_edges_array.elementsMap().clear();
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
    points_edges_states.emplace_back(std::move(settings));

    for (e = edges_.begin(); e != edges_.end(); ++e)
      draw(e, points_edges_array);


    elements_type = 0;
    if (points_edges_array.useElements()) elements_type = points_edges_array.elementsData()->glType();
    vs = points_edges_array.createVertexState(
      GL_LINES, edges_.size() * 2, elements_type,
      points_edges_array.writeIndex(), elements_offset);
    points_edges_states.emplace_back(std::move(vs));
    points_edges_array.addAttributePointers(last_size);

    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
      GL_CHECKD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    GL_CHECKD(glBindBuffer(GL_ARRAY_BUFFER, 0));

    points_edges_array.createInterleavedVBOs();

    // Halffacets
    glGenBuffers(1, &halffacets_vertices_vbo);
    if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
      glGenBuffers(1, &halffacets_elements_vbo);
    }

    // FIXME: We don't know the size of this VertexArray in advanced, so we have to deal with some fallback mechanism for filling in the data. This complicates code quite a bit
    VertexArray halffacets_array(std::make_unique<VertexStateFactory>(), halffacets_states, halffacets_vertices_vbo, halffacets_elements_vbo);
    halffacets_array.addSurfaceData();
    halffacets_array.writeSurface();

    settings = std::make_shared<VertexState>();
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glEnable(GL_LIGHTING)");
      GL_CHECKD(glEnable(GL_LIGHTING));
    });
    settings->glBegin().emplace_back([]() {
      GL_TRACE0("glLineWidth(5.0f)");
      GL_CHECKD(glLineWidth(5.0f));
    });
    if (cull_backfaces || color_backfaces) {
      settings->glBegin().emplace_back([]() {
        GL_TRACE0("glEnable(GL_CULL_FACE)");
        GL_CHECKD(glEnable(GL_CULL_FACE));
      });
      settings->glBegin().emplace_back([]() {
        GL_TRACE0("glCullFace(GL_BACK)");
        GL_CHECKD(glCullFace(GL_BACK));
      });
    }
    halffacets_states.emplace_back(std::move(settings));

    for (int i = 0; i < (color_backfaces ? 2 : 1); i++) {

      Halffacet_iterator f;
      for (f = halffacets_.begin(); f != halffacets_.end(); ++f)
        draw(f, halffacets_array, i);

      if (color_backfaces) {
        settings = std::make_shared<VertexState>();
        settings->glBegin().emplace_back([]() {
          GL_TRACE0("glCullFace(GL_FRONT)");
          GL_CHECKD(glCullFace(GL_FRONT));
        });
        halffacets_states.emplace_back(std::move(settings));
      }
    }

    if (cull_backfaces || color_backfaces) {
      settings = std::make_shared<VertexState>();
      settings->glBegin().emplace_back([]() {
        GL_TRACE0("glCullFace(GL_BACK)");
        GL_CHECKD(glCullFace(GL_BACK));
      });
      settings->glBegin().emplace_back([]() {
        GL_TRACE0("glDisable(GL_CULL_FACE)");
        GL_CHECKD(glDisable(GL_CULL_FACE));
      });
      halffacets_states.emplace_back(std::move(settings));
    }

    halffacets_array.createInterleavedVBOs();
  }

  void init() override {
    PRINTD("VBO init()");
    create_polyhedron();
    PRINTD("VBO init() end");
  }

  void draw() const override {
    PRINTD("VBO draw()");
    PRINTD("VBO draw() end");
  }

protected:
  GLuint points_edges_vertices_vbo{0};
  GLuint points_edges_elements_vbo{0};
  GLuint halffacets_vertices_vbo{0};
  GLuint halffacets_elements_vbo{0};
  std::vector<std::shared_ptr<VertexState>> points_edges_states;
  std::vector<std::shared_ptr<VertexState>> halffacets_states;
}; // Polyhedron
