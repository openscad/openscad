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

class Polyhedron : public CGAL::OGL::Polyhedron
{
public:

	enum RenderColor {
		CGAL_NEF3_MARKED_VERTEX_COLOR,
		CGAL_NEF3_MARKED_EDGE_COLOR,
		CGAL_NEF3_MARKED_FACET_COLOR,
		CGAL_NEF3_UNMARKED_VERTEX_COLOR,
		CGAL_NEF3_UNMARKED_EDGE_COLOR,
		CGAL_NEF3_UNMARKED_FACET_COLOR,
		NUM_COLORS
	};

	Polyhedron() {
		// Set default colors.
		setColor(CGAL_NEF3_MARKED_VERTEX_COLOR,0xb7,0xe8,0x5c);
		setColor(CGAL_NEF3_UNMARKED_VERTEX_COLOR,0xff,0xf6,0x7c);
		//setColor(CGAL_NEF3_MARKED_EDGE_COLOR,0xab,0xd8,0x56);
		//setColor(CGAL_NEF3_UNMARKED_EDGE_COLOR,0xff,0xec,0x5e);
		// Face and Edge colors are taken from default colorscheme
		OSColors::colorscheme cs = RenderSettings::inst()->defaultColorScheme();
		setColorScheme( cs );
		PRINTD("Cgalrender polyhedron constr");
	}

	void draw(bool showedges) const {
		PRINTD("cgal hedron draw");
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
	}
	CGAL::Color getVertexColor(Vertex_iterator v) const {
		CGAL::Color c = v->mark() ? colors[CGAL_NEF3_UNMARKED_VERTEX_COLOR] : colors[CGAL_NEF3_MARKED_VERTEX_COLOR];
		return c;
	}

	CGAL::Color getEdgeColor(Edge_iterator e) const {
		CGAL::Color c = e->mark() ? colors[CGAL_NEF3_UNMARKED_EDGE_COLOR] : colors[CGAL_NEF3_MARKED_EDGE_COLOR];
		return c;
	}

	CGAL::Color getFacetColor(Halffacet_iterator f) const {
		CGAL::Color c = f->mark() ? colors[CGAL_NEF3_UNMARKED_FACET_COLOR] : colors[CGAL_NEF3_MARKED_FACET_COLOR];
		return c;
	}

	void setColor(Polyhedron::RenderColor color_index, const Color4f &c) {
		this->colors[color_index] = CGAL::Color(c[0]*255,c[1]*255,c[2]*255);
	}

	void setColor(Polyhedron::RenderColor color_index,
				  unsigned char r, unsigned char g, unsigned char b) {
		this->colors[color_index] = CGAL::Color(r,g,b);
	}

	// set this->colors based on the given colorscheme. vertex colors
	// are not set here as colorscheme doesnt yet hold vertex colors.
	void setColorScheme(const OSColors::colorscheme &cs) {
		PRINTD("cgal renderer polyhedron setcolsch");
		//setColor(CGAL_NEF3_MARKED_VERTEX_COLOR,cs.at(...));
		//setColor(CGAL_NEF3_UNMARKED_VERTEX_COLOR,cs.at(...));
		setColor(CGAL_NEF3_MARKED_FACET_COLOR, OSColors::getValue(cs,OSColors::CGAL_FACE_BACK_COLOR));
		setColor(CGAL_NEF3_MARKED_EDGE_COLOR, OSColors::getValue(cs,OSColors::CGAL_EDGE_FRONT_COLOR));
		setColor(CGAL_NEF3_UNMARKED_FACET_COLOR, OSColors::getValue(cs,OSColors::CGAL_FACE_FRONT_COLOR));
		setColor(CGAL_NEF3_UNMARKED_EDGE_COLOR, OSColors::getValue(cs,OSColors::CGAL_EDGE_BACK_COLOR));
	}

private:
	CGAL::Color colors[NUM_COLORS];

}; // Polyhedron




#else // NULLGL

#include <CGAL/Bbox_3.h>

class Polyhedron
{
public:
	Polyhedron() {}
	void draw(bool showedges) const {}
	CGAL::Bbox_3 bbox() const { return CGAL::Bbox_3(-1,-1,-1,1,1,1); }
};

#endif // NULLGL
