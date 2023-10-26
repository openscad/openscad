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

#ifndef NULLGL

#include "ColorMap.h"
#include "ext/CGAL/OGL_helper.h"
#include "printutils.h"
#include "VertexArray.h"
#include "VertexState.h"

using namespace CGAL::OGL;

class CGAL_OGL_VBOPolyhedron : public Polyhedron
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

  CGAL_OGL_VBOPolyhedron(const ColorScheme& cs) {
    PRINTD("CGAL_OGL_VBOPolyhedron()");
    // Set default colors.
    setColor(CGALColorIndex::MARKED_VERTEX_COLOR, 0xb7, 0xe8, 0x5c);
    setColor(CGALColorIndex::UNMARKED_VERTEX_COLOR, 0xff, 0xf6, 0x7c);
    // Face and Edge colors are taken from default colorscheme
    setColorScheme(cs);
    PRINTD("CGAL_OGL_VBOPolyhedron() end");
  }

  ~CGAL_OGL_VBOPolyhedron() override {
    if (points_edges_vertices_vbo) glDeleteBuffers(1, &points_edges_vertices_vbo);
    if (points_edges_elements_vbo) glDeleteBuffers(1, &points_edges_elements_vbo);
    if (halffacets_vertices_vbo) glDeleteBuffers(1, &halffacets_vertices_vbo);
    if (halffacets_elements_vbo) glDeleteBuffers(1, &halffacets_elements_vbo);
  }

  void init() override {
    PRINTD("VBO init()");
    create_polyhedron();
    PRINTD("VBO init() end");
  }

  // overrides function in OGL_helper.h
  [[nodiscard]] CGAL::Color getVertexColor(Vertex_iterator v) const override {
    CGAL::Color c = v->mark() ? colors[CGALColorIndex::UNMARKED_VERTEX_COLOR] : colors[CGALColorIndex::MARKED_VERTEX_COLOR];
    return c;
  }

  // overrides function in OGL_helper.h
  [[nodiscard]] CGAL::Color getEdgeColor(Edge_iterator e) const override {
    CGAL::Color c = e->mark() ? colors[CGALColorIndex::UNMARKED_EDGE_COLOR] : colors[CGALColorIndex::MARKED_EDGE_COLOR];
    return c;
  }

  // overrides function in OGL_helper.h
  [[nodiscard]] CGAL::Color getFacetColor(Halffacet_iterator f, bool /*is_back_facing*/) const override {
    CGAL::Color c = f->mark() ? colors[CGALColorIndex::UNMARKED_FACET_COLOR] : colors[CGALColorIndex::MARKED_FACET_COLOR];
    return c;
  }

  void setColor(CGALColorIndex color_index, const Color4f& c) {
    PRINTDB("setColor %i %f %f %f", color_index % c[0] % c[1] % c[2]);
    this->colors[color_index] = CGAL::Color(c[0] * 255, c[1] * 255, c[2] * 255);
  }

  void setColor(CGALColorIndex color_index,
                unsigned char r, unsigned char g, unsigned char b) {
    PRINTDB("setColor %i %i %i %i", color_index % r % g % b);
    this->colors[color_index] = CGAL::Color(r, g, b);
  }

  // set this->colors based on the given colorscheme. vertex colors
  // are not set here as colorscheme doesn't yet hold vertex colors.
  void setColorScheme(const ColorScheme& cs) {
    PRINTD("setColorScheme");
    setColor(CGALColorIndex::MARKED_FACET_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_FACE_BACK_COLOR));
    setColor(CGALColorIndex::UNMARKED_FACET_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_FACE_FRONT_COLOR));
    setColor(CGALColorIndex::MARKED_EDGE_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_EDGE_BACK_COLOR));
    setColor(CGALColorIndex::UNMARKED_EDGE_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_EDGE_FRONT_COLOR));
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

  static inline void CGAL_GLU_TESS_CALLBACK beginCallback(GLenum which, GLvoid *user);
  static inline void CGAL_GLU_TESS_CALLBACK endCallback(GLvoid *user);
  static inline void CGAL_GLU_TESS_CALLBACK errorCallback(GLenum errorCode);
  static inline void CGAL_GLU_TESS_CALLBACK vertexCallback(GLvoid *vertex, GLvoid *user);
  static inline void CGAL_GLU_TESS_CALLBACK combineCallback(GLdouble coords[3], GLvoid *[4], GLfloat [4], GLvoid **dataOut);

  void draw(Vertex_iterator v, VertexArray& vertex_array) const;
  void draw(Edge_iterator e, VertexArray& vertex_array) const;
  void draw(Halffacet_iterator f, VertexArray& vertex_array, bool is_back_facing) const;

  void create_polyhedron();

  void draw(bool showedges) const override;
protected:
  GLuint points_edges_vertices_vbo{0};
  GLuint points_edges_elements_vbo{0};
  GLuint halffacets_vertices_vbo{0};
  GLuint halffacets_elements_vbo{0};
  VertexStates points_edges_states;
  VertexStates halffacets_states;
private:
  CGAL::Color colors[CGALColorIndex::NUM_COLORS];
}; // CGAL_OGL_VBOPolyhedron




#else // NULLGL

#include <CGAL/Bbox_3.h>

class CGAL_OGL_VBOPolyhedron : public CGAL::OGL::Polyhedron
{
public:
  CGAL_OGL_VBOPolyhedron() {}
  void draw(bool showedges) const {}
  CGAL::Bbox_3 bbox() const { return CGAL::Bbox_3(-1, -1, -1, 1, 1, 1); }
};

#endif // NULLGL
