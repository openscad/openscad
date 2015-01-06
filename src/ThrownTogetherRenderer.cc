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

#include "ThrownTogetherRenderer.h"
#include "polyset.h"
#include "csgterm.h"
#include "printutils.h"

#include "system-gl.h"

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

ThrownTogetherRenderer::ThrownTogetherRenderer(CSGChain *root_chain, 
																							 CSGChain *highlights_chain,
																							 CSGChain *background_chain)
	: root_chain(root_chain), highlights_chain(highlights_chain), 
		background_chain(background_chain)
{
}

void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	PRINTD("Thrown draw");
	if (this->root_chain) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		renderCSGChain(this->root_chain, false, false, showedges, false);
		glCullFace(GL_FRONT);
		glColor3ub(255, 0, 255);
		renderCSGChain(this->root_chain, false, false, showedges, true);
		glDisable(GL_CULL_FACE);
	}
	if (this->background_chain)
	 	renderCSGChain(this->background_chain, false, true, showedges, false);
	if (this->highlights_chain)
	 	renderCSGChain(this->highlights_chain, true, false, showedges, false);
}

void ThrownTogetherRenderer::renderCSGChain(CSGChain *chain, bool highlight,
																						bool background, bool showedges, 
																						bool fberror) const
{
	PRINTD("Thrown renderCSGChain");
	glDepthFunc(GL_LEQUAL);
	boost::unordered_map<std::pair<const Geometry*,const Transform3d*>,int> geomVisitMark;
	BOOST_FOREACH(const CSGChainObject &obj, chain->objects) {
		if (geomVisitMark[std::make_pair(obj.geom.get(), &obj.matrix)]++ > 0)
			continue;
		const Transform3d &m = obj.matrix;
		const Color4f &c = obj.color;
		glPushMatrix();
		glMultMatrixd(m.data());
		csgmode_e csgmode = csgmode_e(
			(highlight ? 
			 CSGMODE_HIGHLIGHT :
			 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
			(obj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0));
		ColorMode colormode = COLORMODE_NONE;
		ColorMode edge_colormode = COLORMODE_NONE;

		if (highlight) {
			colormode = COLORMODE_HIGHLIGHT;
			edge_colormode = COLORMODE_HIGHLIGHT_EDGES;
		} else if (background) {
			if (obj.flag & CSGTerm::FLAG_HIGHLIGHT) {
				colormode = COLORMODE_HIGHLIGHT;
			}
			else {
				colormode = COLORMODE_BACKGROUND;
			}
			edge_colormode = COLORMODE_BACKGROUND_EDGES;
		} else if (fberror) {
		} else if (obj.type == CSGTerm::TYPE_DIFFERENCE) {
			if (obj.flag & CSGTerm::FLAG_HIGHLIGHT) {
				colormode = COLORMODE_HIGHLIGHT;
			}
			else {
				colormode = COLORMODE_CUTOUT;
			}
			edge_colormode = COLORMODE_CUTOUT_EDGES;
		} else {
			if (obj.flag & CSGTerm::FLAG_HIGHLIGHT) {
				colormode = COLORMODE_HIGHLIGHT;
			}
			else {
				colormode = COLORMODE_MATERIAL;
			}
			edge_colormode = COLORMODE_MATERIAL_EDGES;
		}
		
		setColor(colormode, c.data());
		render_surface(obj.geom, csgmode, m);
		if (showedges) {
			// FIXME? glColor4f((c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
			setColor(edge_colormode);
			render_edges(obj.geom, csgmode);
		}

		glPopMatrix();
	}
}

BoundingBox ThrownTogetherRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_chain) bbox = this->root_chain->getBoundingBox();
	return bbox;
}
