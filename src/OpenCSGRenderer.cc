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

#include <GL/glew.h>
#include "OpenCSGRenderer.h"
#include "polyset.h"
#include "csgterm.h"
#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	PolySet *p;
	double *m;
	int csgmode;
	virtual void render() {
		glPushMatrix();
		glMultMatrixd(m);
		p->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m);
		glPopMatrix();
	}
};

OpenCSGRenderer::OpenCSGRenderer(CSGChain *root_chain, CSGChain *highlights_chain,
																 CSGChain *background_chain, GLint *shaderinfo)
	: root_chain(root_chain), highlights_chain(highlights_chain), 
		background_chain(background_chain), shaderinfo(shaderinfo)
{
}

void OpenCSGRenderer::draw(bool showfaces, bool showedges) const
{
	if (this->root_chain) {
		GLint *shaderinfo = this->shaderinfo;
		if (!shaderinfo[0]) shaderinfo = NULL;
		renderCSGChain(this->root_chain, showedges ? shaderinfo : NULL, false, false);
		if (this->background_chain) {
			renderCSGChain(this->background_chain, showedges ? shaderinfo : NULL, false, true);
		}
		if (this->highlights_chain) {
			renderCSGChain(this->highlights_chain, showedges ? shaderinfo : NULL, true, false);
		}
	}
}

void OpenCSGRenderer::renderCSGChain(CSGChain *chain, GLint *shaderinfo, 
																		 bool highlight, bool background) const
{
	std::vector<OpenCSG::Primitive*> primitives;
	int j = 0;
	for (int i = 0;; i++) {
		bool last = i == chain->polysets.size();
		if (last || chain->types[i] == CSGTerm::TYPE_UNION) {
			if (j+1 != i) {
				 OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			if (shaderinfo) glUseProgram(shaderinfo[0]);
			for (; j < i; j++) {
				double *m = chain->matrices[j];
				glPushMatrix();
				glMultMatrixd(m);
				int csgmode = chain->types[j] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
				if (highlight) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20), m, shaderinfo);
				} else if (background) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10), m, shaderinfo);
				} else if (m[16] >= 0 || m[17] >= 0 || m[18] >= 0) {
					// User-defined color from source
					glColor4d(m[16], m[17], m[18], m[19]);
					if (shaderinfo) {
						glUniform4f(shaderinfo[1], m[16], m[17], m[18], m[19]);
						glUniform4f(shaderinfo[2], (m[16]+1)/2, (m[17]+1)/2, (m[18]+1)/2, 1.0);
					}
					chain->polysets[j]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m, shaderinfo);
				} else if (chain->types[j] == CSGTerm::TYPE_DIFFERENCE) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode), m, shaderinfo);
				} else {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode), m, shaderinfo);
				}
				glPopMatrix();
			}
			if (shaderinfo) glUseProgram(0);
			for (unsigned int k = 0; k < primitives.size(); k++) {
				delete primitives[k];
			}
			glDepthFunc(GL_LEQUAL);
			primitives.clear();
		}

		if (last) break;

		OpenCSGPrim *prim = new OpenCSGPrim(chain->types[i] == CSGTerm::TYPE_DIFFERENCE ?
				OpenCSG::Subtraction : OpenCSG::Intersection, chain->polysets[i]->convexity);
		prim->p = chain->polysets[i];
		prim->m = chain->matrices[i];
		prim->csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight) prim->csgmode += 20;
		else if (background) prim->csgmode += 10;
		primitives.push_back(prim);
	}
}
