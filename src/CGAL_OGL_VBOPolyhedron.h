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

#include "colormap.h"
#include "rendersettings.h"
#include "CGAL_OGL_Polyhedron.h"
#include "ext/CGAL/OGL_VBO_helper.h"
#include "printutils.h"

class CGAL_OGL_VBOPolyhedron : public CGAL::OGL::VBOPolyhedron, public CGAL_OGL_Polyhedron
{
public:

	CGAL_OGL_VBOPolyhedron(const ColorScheme &cs)
		: CGAL::OGL::VBOPolyhedron(), CGAL_OGL_Polyhedron(cs) {
		PRINTD("CGAL_OGL_VBOPolyhedron()");
		PRINTD("CGAL_OGL_VBOPolyhedron() end");
	}

	void draw(bool showedges) const override {
		PRINTD("VBO draw()");
		// grab current state to restore after
		GLfloat current_point_size, current_line_width;
		GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
		GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
		GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

		glGetFloatv(GL_POINT_SIZE, &current_point_size);
		glGetFloatv(GL_LINE_WIDTH, &current_line_width);

		if(this->style == SNC_BOUNDARY) {
			for (auto &halffacet : this->polyhedron_halffacets_) {
				glBindBuffer(GL_ARRAY_BUFFER, halffacet.vbo);
				glEnable(GL_LIGHTING);
				if (halffacet.vertex_set->draw_cull_front || halffacet.vertex_set->draw_cull_back) {
					glEnable(GL_CULL_FACE);
					if (halffacet.vertex_set->draw_cull_front) {
				        	glCullFace(GL_BACK);
					} else if (halffacet.vertex_set->draw_cull_front) {
						glCullFace(GL_FRONT);			
					}
				}
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);
				glVertexPointer(3, GL_FLOAT, sizeof(VBORenderer::PNCVertex), (GLvoid *)(0));
				glNormalPointer(GL_FLOAT, sizeof(VBORenderer::PNCVertex), (GLvoid *)(sizeof(float)*3));
				glColorPointer(4, GL_FLOAT, sizeof(VBORenderer::PNCVertex), (GLvoid *)((sizeof(float)*3)*2));
				
				glDrawArrays(halffacet.vertex_set->draw_type, 0, halffacet.vertex_set->draw_size);
				if (halffacet.vertex_set->draw_cull_front || halffacet.vertex_set->draw_cull_back) {
					glDisable(GL_CULL_FACE);
				}
			}
		}
		
		if (this->style != SNC_BOUNDARY || showedges) {
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHTING);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glLineWidth(5.0f);
			glBindBuffer(GL_ARRAY_BUFFER, this->polyhedron_edges_.vbo);
			glVertexPointer(3, GL_FLOAT, sizeof(VBORenderer::PCVertex), (GLvoid *)(0));
			glColorPointer(4, GL_FLOAT, sizeof(VBORenderer::PCVertex), (GLvoid *)(sizeof(float)*3));
			glDrawArrays(this->polyhedron_edges_.vertex_set->draw_type, 0, this->polyhedron_edges_.vertex_set->draw_size);

			glPointSize(10.0f);
			glBindBuffer(GL_ARRAY_BUFFER, this->polyhedron_vertices_.vbo);
			glVertexPointer(3, GL_FLOAT, sizeof(VBORenderer::PCVertex), (GLvoid *)(0));
			glColorPointer(4, GL_FLOAT, sizeof(VBORenderer::PCVertex), (GLvoid *)(sizeof(float)*3));
			glDrawArrays(this->polyhedron_vertices_.vertex_set->draw_type, 0, this->polyhedron_vertices_.vertex_set->draw_size);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// restore states
		glPointSize(current_point_size);
		glLineWidth(current_line_width);

		if (!origVertexArrayState) glDisableClientState(GL_VERTEX_ARRAY);
		if (!origNormalArrayState) glDisableClientState(GL_NORMAL_ARRAY);
		if (!origColorArrayState) glDisableClientState(GL_COLOR_ARRAY);

		PRINTD("VBO draw() end");
	}
	
}; // CGAL_OGL_Polyhedron




#else // NULLGL

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <CGAL/Bbox_3.h>
#pragma pop_macro("NDEBUG")

class CGAL_OGL_VBOPolyhedron : public CGAL::OGL::VBOPolyhedron, public CGAL_OGL_Polyhedron
{
public:
	CGAL_OGL_VBOPolyhedron() {}
	void draw(bool showedges) const {}
	CGAL::Bbox_3 bbox() const { return CGAL::Bbox_3(-1,-1,-1,1,1,1); }
};

#endif // NULLGL
