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
#include "OGL_helper.h"
#include "printutils.h"

using CGAL::OGL::SNC_BOUNDARY;
using CGAL::OGL::SNC_SKELETON;

class CGAL_OGL_Polyhedron : public CGAL::OGL::Polyhedron
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

	CGAL_OGL_Polyhedron(const ColorScheme &cs) {
		PRINTD("CGAL_OGL_Polyhedron()");
		// Set default colors.
		setColor(CGALColorIndex::MARKED_VERTEX_COLOR,0xb7,0xe8,0x5c);
		setColor(CGALColorIndex::UNMARKED_VERTEX_COLOR,0xff,0xf6,0x7c);
		// Face and Edge colors are taken from default colorscheme
		setColorScheme(cs);
		PRINTD("CGAL_OGL_Polyhedron() end");
	}

	void draw(bool showedges) const {
		PRINTD("draw()");
		if(this->style == SNC_BOUNDARY) {
			glCallList(this->object_list_+2);
			if(showedges) {
				glDisable(GL_LIGHTING);
				glCallList(this->object_list_+1);
				glCallList(this->object_list_);
			}
		} else {
			glDisable(GL_LIGHTING);
			glCallList(this->object_list_+1);
			glCallList(this->object_list_);
		}
		PRINTD("draw() end");
	}

	// overrides function in OGL_helper.h
	CGAL::Color getVertexColor(Vertex_iterator v) const {
		PRINTD("getVertexColor");
		CGAL::Color c = v->mark() ? colors[CGALColorIndex::UNMARKED_VERTEX_COLOR] : colors[CGALColorIndex::MARKED_VERTEX_COLOR];
		return c;
	}

	// overrides function in OGL_helper.h
	CGAL::Color getEdgeColor(Edge_iterator e) const {
		PRINTD("getEdgeColor");
		CGAL::Color c = e->mark() ? colors[CGALColorIndex::UNMARKED_EDGE_COLOR] : colors[CGALColorIndex::MARKED_EDGE_COLOR];
		return c;
	}

	// overrides function in OGL_helper.h
	CGAL::Color getFacetColor(Halffacet_iterator f, bool is_back_facing) const {
		PRINTD("getFacetColor");
		CGAL::Color c = f->mark() ? colors[CGALColorIndex::UNMARKED_FACET_COLOR] : colors[CGALColorIndex::MARKED_FACET_COLOR];
		return c;
	}

	void setColor(CGAL_OGL_Polyhedron::CGALColorIndex color_index, const Color4f &c) {
		PRINTDB("setColor %i %f %f %f",color_index%c[0]%c[1]%c[2]);
		this->colors[color_index] = CGAL::Color(c[0]*255,c[1]*255,c[2]*255);
	}

	void setColor(CGAL_OGL_Polyhedron::CGALColorIndex color_index,
				  unsigned char r, unsigned char g, unsigned char b) {
		PRINTDB("setColor %i %i %i %i",color_index%r%g%b);
		this->colors[color_index] = CGAL::Color(r,g,b);
	}

	// set this->colors based on the given colorscheme. vertex colors
	// are not set here as colorscheme doesnt yet hold vertex colors.
	void setColorScheme(const ColorScheme &cs) {
		PRINTD("setColorScheme");
		setColor(CGALColorIndex::MARKED_FACET_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_FACE_BACK_COLOR));
		setColor(CGALColorIndex::UNMARKED_FACET_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_FACE_FRONT_COLOR));
		setColor(CGALColorIndex::MARKED_EDGE_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_EDGE_BACK_COLOR));
		setColor(CGALColorIndex::UNMARKED_EDGE_COLOR, ColorMap::getColor(cs, RenderColor::CGAL_EDGE_FRONT_COLOR));
	}

private:
	CGAL::Color colors[CGALColorIndex::NUM_COLORS];

}; // CGAL_OGL_Polyhedron




#else // NULLGL

#include <CGAL/Bbox_3.h>

class CGAL_OGL_Polyhedron
{
public:
	CGAL_OGL_Polyhedron() {}
	void draw(bool showedges) const {}
	CGAL::Bbox_3 bbox() const { return CGAL::Bbox_3(-1,-1,-1,1,1,1); }
};

#endif // NULLGL
