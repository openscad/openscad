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
<<<<<<< eba4a4f817553bf728c7b8af6e8493e8e0a68cc9

=======
#include <boost/foreach.hpp>
>>>>>>> Initial attempt at refactoring CSG product rendering
#ifdef ENABLE_OPENCSG
#  include <opencsg.h>

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	shared_ptr<const Geometry> geom;
	Transform3d m;
	Renderer::csgmode_e csgmode;
	virtual void render() {
		glPushMatrix();
		glMultMatrixd(m.data());
		Renderer::render_surface(geom, csgmode, m);
		glPopMatrix();
	}
};

#endif

OpenCSGRenderer::OpenCSGRenderer(CSGChain *root_chain, CSGProducts *root_products, CSGChain *highlights_chain,
																 CSGChain *background_chain, GLint *shaderinfo)
	: root_chain(root_chain), root_products(root_products), highlights_chain(highlights_chain), 
		background_chain(background_chain), shaderinfo(shaderinfo)
{
}

OpenCSGRenderer::OpenCSGRenderer(CSGChain *root_chain, CSGChain *highlights_chain,
																 CSGChain *background_chain, GLint *shaderinfo)
	: root_chain(root_chain), root_products(NULL), highlights_chain(highlights_chain), 
		background_chain(background_chain), shaderinfo(shaderinfo)
{
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	GLint *shaderinfo = this->shaderinfo;
	if (!shaderinfo[0]) shaderinfo = NULL;
	if (this->root_chain) {
		renderCSGChain(this->root_chain, showedges ? shaderinfo : NULL, false, false);
	}
	if (this->background_chain) {
		renderCSGChain(this->background_chain, showedges ? shaderinfo : NULL, false, true);
	}
	if (this->highlights_chain) {
		renderCSGChain(this->highlights_chain, showedges ? shaderinfo : NULL, true, false);
	}
}

OpenCSGPrim *OpenCSGRenderer::createCSGPrimitive(const CSGChainObject &csgobj, OpenCSG::Operation operation) const
{
	OpenCSGPrim *prim = new OpenCSGPrim(operation, csgobj.geom->getConvexity());
	prim->geom = csgobj.geom;
	prim->m = csgobj.matrix;
	prim->csgmode = csgmode_e(csgobj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0);
	return prim;
}

void OpenCSGRenderer::renderCSGProducts(const CSGProducts &products, GLint *shaderinfo, 
																				bool highlight, bool background) const
{
#ifdef ENABLE_OPENCSG
	std::vector<OpenCSG::Primitive*> primitives;
	BOOST_FOREACH(const CSGProduct &product, products.products) {
		BOOST_FOREACH(const CSGChainObject &csgobj, product.intersections) {
			primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Intersection));
		}
		BOOST_FOREACH(const CSGChainObject &csgobj, product.subtractions) {
			primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Subtraction));
		}
		OpenCSG::render(primitives);

		glDepthFunc(GL_EQUAL);
			if (shaderinfo) glUseProgram(shaderinfo[0]);

		BOOST_FOREACH(const CSGChainObject &csgobj, product.intersections) {
			const Color4f &c = csgobj.color;
			glPushMatrix();
			glMultMatrixd(csgobj.matrix.data());
			csgmode_e csgmode = csgmode_e(
				(highlight ? 
				 CSGMODE_HIGHLIGHT :
				 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)));
			
			ColorMode colormode = COLORMODE_NONE;
			if (background) {
				if (csgobj.flag & CSGTerm::FLAG_HIGHLIGHT) {
					colormode = COLORMODE_HIGHLIGHT;
				}
				else {
					colormode = COLORMODE_BACKGROUND;
				}
			} else if (csgobj.flag & CSGTerm::FLAG_HIGHLIGHT) {
				colormode = COLORMODE_HIGHLIGHT;
			}
			else {
				colormode = COLORMODE_MATERIAL;
			}
			
			setColor(colormode, c.data(), shaderinfo);
			
			render_surface(csgobj.geom, csgmode, csgobj.matrix, shaderinfo);
			glPopMatrix();
		}
		BOOST_FOREACH(const CSGChainObject &csgobj, product.subtractions) {
			const Color4f &c = csgobj.color;
			glPushMatrix();
			glMultMatrixd(csgobj.matrix.data());
			csgmode_e csgmode = csgmode_e(
				(highlight ? 
				 CSGMODE_HIGHLIGHT :
				 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) | CSGMODE_DIFFERENCE);
			
			ColorMode colormode = COLORMODE_NONE;
			if (background) {
				if (csgobj.flag & CSGTerm::FLAG_HIGHLIGHT) {
					colormode = COLORMODE_HIGHLIGHT;
				}
				else {
					colormode = COLORMODE_BACKGROUND;
				}
			} else {
				if (csgobj.flag & CSGTerm::FLAG_HIGHLIGHT) {
					colormode = COLORMODE_HIGHLIGHT;
				}
				else {
					colormode = COLORMODE_CUTOUT;
				}
			}
			
			setColor(colormode, c.data(), shaderinfo);
			
			render_surface(csgobj.geom, csgmode, csgobj.matrix, shaderinfo);
			glPopMatrix();
		}

		if (shaderinfo) glUseProgram(0);
		BOOST_FOREACH(OpenCSG::Primitive *p, primitives) delete p;
		primitives.clear();
		glDepthFunc(GL_LEQUAL);
	}
#endif
}

void OpenCSGRenderer::renderCSGChain(CSGChain *chain, GLint *shaderinfo, 
									 bool highlight, bool background) const
{
#ifdef ENABLE_OPENCSG
	std::vector<OpenCSG::Primitive*> primitives;
	size_t j = 0;
	for (size_t i = 0;; i++) {
		bool last = i == chain->objects.size();
		const CSGChainObject &i_obj = last ? chain->objects[i-1] : chain->objects[i];
		if ((last || i_obj.type == CSGTerm::TYPE_UNION) && (i != 0)) {
			if (j+1 != i) {
				 OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			if (shaderinfo) glUseProgram(shaderinfo[0]);
			const CSGChainObject &parent_obj = chain->objects[j];
			for (; j < i; j++) {
				const CSGChainObject &j_obj = chain->objects[j];
				const Color4f &c = j_obj.color;
				csgmode_e csgmode = csgmode_e(
					(highlight ? 
					 CSGMODE_HIGHLIGHT :
					 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
					(j_obj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0));

				ColorMode colormode = COLORMODE_NONE;
				if (highlight) {
					colormode = COLORMODE_HIGHLIGHT;
				} else if (background) {
					colormode = COLORMODE_BACKGROUND;
				} else if (j_obj.type == CSGTerm::TYPE_DIFFERENCE) {
					colormode = COLORMODE_CUTOUT;
				} else {
					colormode = COLORMODE_MATERIAL;
				}

				if (highlight || !(parent_obj.flag & CSGTerm::FLAG_HIGHLIGHT)) {
					setColor(colormode, c.data(), shaderinfo);

					glPushMatrix();
					glMultMatrixd(j_obj.matrix.data());
					render_surface(j_obj.geom, csgmode, j_obj.matrix, shaderinfo);
					glPopMatrix();
				}
			}
			
			if (shaderinfo) glUseProgram(0);
			for (unsigned int k = 0; k < primitives.size(); k++) {
				delete primitives[k];
			}
			glDepthFunc(GL_LEQUAL);
			primitives.clear();
		}

		if (last) break;

		if (i_obj.geom) {
			OpenCSGPrim *prim = new OpenCSGPrim(i_obj.type == CSGTerm::TYPE_DIFFERENCE ?
												OpenCSG::Subtraction : OpenCSG::Intersection, i_obj.geom->getConvexity());
			
			prim->geom = i_obj.geom;
			prim->m = i_obj.matrix;
			prim->csgmode = csgmode_e(
				(highlight ? 
				 CSGMODE_HIGHLIGHT :
				 (background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
				(i_obj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0));

			primitives.push_back(prim);
		}
	}
	std::for_each(primitives.begin(), primitives.end(), del_fun<OpenCSG::Primitive>());
#endif
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_chain) bbox = this->root_chain->getBoundingBox();
	if (this->background_chain) bbox.extend(this->background_chain->getBoundingBox());

	return bbox;
}
