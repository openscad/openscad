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

#include "system-gl.h"
#include "OpenCSGRenderer.h"
#include "polyset.h"
#include "csgterm.h"
#include "stl-utils.h"
#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	boost::shared_ptr<PolySet> ps;
	Transform3d m;
	PolySet::csgmode_e csgmode;
	virtual void render() {
		glPushMatrix();
		glMultMatrixd(m.data());
		ps->render_surface(csgmode, m);
		glPopMatrix();
	}
};

OpenCSGRenderer::OpenCSGRenderer(CSGChain *root_chain, CSGChain *highlights_chain,
																 CSGChain *background_chain, GLint *shaderinfo)
	: root_chain(root_chain), highlights_chain(highlights_chain), 
		background_chain(background_chain), shaderinfo(shaderinfo)
{
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
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

#include "CGALEvaluator.h"
#include "CSGTermEvaluator.h"
#include "csgtermnormalizer.h"

void OpenCSGRenderer::renderCSGChain(CSGChain *chain, GLint *shaderinfo, 
																		 bool highlight, bool background) const
{
	std::vector<OpenCSG::Primitive*> primitives;
	size_t j = 0;
	for (size_t i = 0;; i++) {
		bool last = i == chain->polysets.size();
		if (last || chain->types[i] == CSGTerm::TYPE_UNION) {
			if (j+1 != i) {
				 OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			if (shaderinfo) glUseProgram(shaderinfo[0]);
			for (; j < i; j++) {
				const Transform3d &m = chain->matrices[j];
				const Color4f &c = chain->colors[j];
				glPushMatrix();
				glMultMatrixd(m.data());
				PolySet::csgmode_e csgmode = chain->types[j] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
				if (highlight) {
					setColor(COLORMODE_HIGHLIGHT, shaderinfo);
					csgmode = PolySet::csgmode_e(csgmode + 20);
				}
				else if (background) {
					setColor(COLORMODE_BACKGROUND, shaderinfo);
					csgmode = PolySet::csgmode_e(csgmode + 10);
				} else if (c[0] >= 0 || c[1] >= 0 || c[2] >= 0 || c[3] >= 0) {
					// User-defined color or alpha from source
					setColor(c.data(), shaderinfo);
				} else if (chain->types[j] == CSGTerm::TYPE_DIFFERENCE) {
					setColor(COLORMODE_CUTOUT, shaderinfo);
				} else {
					setColor(COLORMODE_MATERIAL, shaderinfo);
				}
				chain->polysets[j]->render_surface(csgmode, m, shaderinfo);
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
		prim->ps = chain->polysets[i];
		prim->m = chain->matrices[i];
		prim->csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight) prim->csgmode = PolySet::csgmode_e(prim->csgmode + 20);
		else if (background) prim->csgmode = PolySet::csgmode_e(prim->csgmode + 10);
		primitives.push_back(prim);
	}
	std::for_each(primitives.begin(), primitives.end(), del_fun<OpenCSG::Primitive>());
}

// miscellaneous code to prep OpenCSG render.
// returns 0 on success, 1 on failure
// csgInfo gets modified
int opencsg_prep( const Tree &tree, const AbstractNode *root_node, CsgInfo_OpenCSG &csgInfo )
{
	CGALEvaluator cgalevaluator(tree);
	CSGTermEvaluator evaluator(tree, &cgalevaluator.psevaluator);
	boost::shared_ptr<CSGTerm> root_raw_term = evaluator.evaluateCSGTerm(*root_node, 
																																csgInfo.highlight_terms, 
																																csgInfo.background_terms);

	if (!root_raw_term) {
		std::cerr << "Error: CSG generation failed! (no top level object found)\n";
		return 1;
	}

	// CSG normalization
	CSGTermNormalizer normalizer(5000);
	csgInfo.root_norm_term = normalizer.normalize(root_raw_term);
	if (csgInfo.root_norm_term) {
		csgInfo.root_chain = new CSGChain();
		csgInfo.root_chain->import(csgInfo.root_norm_term);
		fprintf(stderr, "Normalized CSG tree has %d elements\n", int(csgInfo.root_chain->polysets.size()));
	}
	else {
		csgInfo.root_chain = NULL;
		fprintf(stderr, "WARNING: CSG normalization resulted in an empty tree\n");
	}

	if (csgInfo.highlight_terms.size() > 0) {
		std::cerr << "Compiling highlights (" << csgInfo.highlight_terms.size() << " CSG Trees)...\n";
		
		csgInfo.highlights_chain = new CSGChain();
		for (unsigned int i = 0; i < csgInfo.highlight_terms.size(); i++) {
			csgInfo.highlight_terms[i] = normalizer.normalize(csgInfo.highlight_terms[i]);
			csgInfo.highlights_chain->import(csgInfo.highlight_terms[i]);
		}
	}
	
	if (csgInfo.background_terms.size() > 0) {
		std::cerr << "Compiling background (" << csgInfo.background_terms.size() << " CSG Trees)...\n";
		
		csgInfo.background_chain = new CSGChain();
		for (unsigned int i = 0; i < csgInfo.background_terms.size(); i++) {
			csgInfo.background_terms[i] = normalizer.normalize(csgInfo.background_terms[i]);
			csgInfo.background_chain->import(csgInfo.background_terms[i]);
		}
	}
	return 0;
}
