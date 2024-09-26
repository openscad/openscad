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

#include "glview/ColorMap.h"
#include "glview/cgal/CGAL_OGL_Polyhedron.h"
#include "glview/cgal/CGAL_OGL_VBO_helper.h"
#include "utils/printutils.h"

class CGAL_OGL_VBOPolyhedron : public VBOPolyhedron, public CGAL_OGL_Polyhedron
{
public:

  CGAL_OGL_VBOPolyhedron(const ColorScheme& cs)
    : VBOPolyhedron(), CGAL_OGL_Polyhedron(cs) {
    PRINTD("CGAL_OGL_VBOPolyhedron()");
    PRINTD("CGAL_OGL_VBOPolyhedron() end");
  }

  using VBOPolyhedron::draw; // draw()
  void draw(bool showedges) const override {
    PRINTDB("VBO draw(showedges = %d)", showedges);
    // grab current state to restore after
    GLfloat current_point_size, current_line_width;
    GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
    GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
    GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

    GL_CHECKD(glGetFloatv(GL_POINT_SIZE, &current_point_size));
    GL_CHECKD(glGetFloatv(GL_LINE_WIDTH, &current_line_width));

    if (this->style == SNC_BOUNDARY) {
      for (const auto& halffacet : this->halffacets_states) {
        if (halffacet) halffacet->draw();
      }
    }

    if (this->style != SNC_BOUNDARY || showedges) {
      for (const auto& point_edge : this->points_edges_states) {
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
}; // CGAL_OGL_Polyhedron




#else // NULLGL

#include <CGAL/Bbox_3.h>

class CGAL_OGL_VBOPolyhedron : public CGAL::OGL::VBOPolyhedron, public CGAL_OGL_Polyhedron
{
public:
  CGAL_OGL_VBOPolyhedron() {}
  void draw(bool showedges) const {}
  CGAL::Bbox_3 bbox() const { return CGAL::Bbox_3(-1, -1, -1, 1, 1, 1); }
};

#endif // NULLGL
