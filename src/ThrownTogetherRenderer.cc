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

#include "system-gl.h"

#include <boost/unordered_map.hpp>

ThrownTogetherRenderer::ThrownTogetherRenderer(CSGChain *root_chain, 
																							 CSGChain *highlights_chain,
																							 CSGChain *background_chain)
	: root_chain(root_chain), highlights_chain(highlights_chain), 
		background_chain(background_chain)
{
}

void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges) const
{
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
	glDepthFunc(GL_LEQUAL);
	boost::unordered_map<std::pair<PolySet*,Transform3d*>,int> polySetVisitMark;
	for (size_t i = 0; i < chain->polysets.size(); i++) {
		if (polySetVisitMark[std::make_pair(chain->polysets[i].get(), &chain->matrices[i])]++ > 0)
			continue;
		const Transform3d &m = chain->matrices[i];
		double *c = chain->colors[i];
		glPushMatrix();
		glMultMatrixd(m.data());
		int csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20), m);
			if (showedges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20));
				glEnable(GL_LIGHTING);
			}
		} else if (background) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10), m);
			if (showedges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10));
				glEnable(GL_LIGHTING);
			}
		} else if (fberror) {
			if (highlight) {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode + 20), m);
			} else if (background) {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode + 10), m);
			} else {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m);
			}
		} else if (c[0] >= 0 || c[1] >= 0 || c[2] >= 0) {
			glColor4dv(c);
			chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m);
			if (showedges) {
				glDisable(GL_LIGHTING);
				glColor4d((c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		} else if (chain->types[i] == CSGTerm::TYPE_DIFFERENCE) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode), m);
			if (showedges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		} else {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode), m);
			if (showedges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		}
		glPopMatrix();
	}
}
