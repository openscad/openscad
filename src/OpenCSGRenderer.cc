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
#include "csgnode.h"
#include "stl-utils.h"
#include <boost/foreach.hpp>
#include "printutils.h"

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>

OpenCSGPrim::~OpenCSGPrim()
{
	if(id_ && built_) {
		glDeleteLists(id_, 1);
	}
}

void OpenCSGPrim::render()
{
	if (!built_) {
		id_ = glGenLists(1);
		glNewList(id_, GL_COMPILE);
			Renderer::render_surface(geom_, csgmode_, m_, shaderinfo_);
		glEndList();
		built_ = true;
	}

	glPushMatrix();
		glMultMatrixd(m_.data());
		glCallList(id_);
	glPopMatrix();
}
#endif

OpenCSGRenderer::OpenCSGRenderer(shared_ptr<CSGProducts> root_products,
																 shared_ptr<CSGProducts> highlights_products,
																 shared_ptr<CSGProducts> background_products,
																 GLint *shaderinfo)
	: root_products(root_products), 
		highlights_products(highlights_products), 
		background_products(background_products), shaderinfo(shaderinfo),
	  root_products_built_(false), highlights_products_built_(false), background_products_built_(false)
{
}

OpenCSGRenderer::~OpenCSGRenderer()
{
	if (root_products_built_){
		for (size_t i = 0; i < root_products_primitives_.size(); i++) {
			std::for_each(root_products_primitives_[i].begin(), root_products_primitives_[i].end(), del_fun<OpenCSG::Primitive>());
		}
		root_products_primitives_.clear();
		for (size_t i = 0; i < root_products_ids_.size(); i++) {
			glDeleteLists(root_products_ids_[i],1);
		}
		root_products_ids_.clear();
	}
	if (highlights_products_built_){
		for (size_t i = 0; i < highlights_products_primitives_.size(); i++) {
			std::for_each(highlights_products_primitives_[i].begin(), highlights_products_primitives_[i].end(), del_fun<OpenCSG::Primitive>());
		}
		highlights_products_primitives_.clear();
		for (size_t i = 0; i < highlights_products_ids_.size(); i++) {
			glDeleteLists(highlights_products_ids_[i],1);
		}
		highlights_products_ids_.clear();
	}
	if (background_products_built_){
		for (size_t i = 0; i < background_products_primitives_.size(); i++) {
			std::for_each(background_products_primitives_[i].begin(), background_products_primitives_[i].end(), del_fun<OpenCSG::Primitive>());
		}
		background_products_primitives_.clear();
		for (size_t i = 0; i < background_products_ids_.size(); i++) {
			glDeleteLists(background_products_ids_[i],1);
		}
		background_products_ids_.clear();
	}
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	GLint *shaderinfo = shaderinfo;
	if (!shaderinfo[0]) shaderinfo = NULL;
	if (root_products) {
		renderCSGProducts(*root_products, showedges ? shaderinfo : NULL, false, false);
	}
	if (background_products) {
		renderCSGProducts(*background_products, showedges ? shaderinfo : NULL, false, true);
	}
	if (highlights_products) {
		renderCSGProducts(*highlights_products, showedges ? shaderinfo : NULL, true, false);
	}
}

// Primitive for rendering using OpenCSG
OpenCSGPrim *OpenCSGRenderer::createCSGPrimitive(const CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type, GLint *shaderinfo) const
{
	OpenCSGPrim *prim = new OpenCSGPrim(csgobj.leaf->geom, csgobj.leaf->matrix,
					    csgmode_e(
						(highlight_mode ?
							CSGMODE_HIGHLIGHT :
								(background_mode ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
									(type == OPENSCAD_DIFFERENCE ? CSGMODE_DIFFERENCE : CSGMODE_NONE)),
					    operation, csgobj.leaf->geom->getConvexity(), shaderinfo);
	return prim;
}

void OpenCSGRenderer::renderCSGProducts(const CSGProducts &products, GLint *shaderinfo,
					bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
	std::vector<std::vector<OpenCSG::Primitive *> > *products_primitives;
	std::vector<unsigned int> *products_ids;
	bool built;

	if (highlight_mode && !background_mode) {
		products_primitives = const_cast<std::vector<std::vector<OpenCSG::Primitive *> > *>(&highlights_products_primitives_);
		products_ids = const_cast<std::vector<unsigned int>*>(&highlights_products_ids_);
		built = highlights_products_built_;
	} else if (!highlight_mode && background_mode) {
		products_primitives = const_cast<std::vector<std::vector<OpenCSG::Primitive *> > *>(&background_products_primitives_);
		products_ids = const_cast<std::vector<unsigned int>*>(&background_products_ids_);
		built = background_products_built_;
	} else {
		products_primitives = const_cast<std::vector<std::vector<OpenCSG::Primitive *> > *>(&root_products_primitives_);
		products_ids = const_cast<std::vector<unsigned int>*>(&root_products_ids_);
		built = root_products_built_;
	}

	size_t opencsg_ids_idx = 0;
	size_t direct_ids_idx = 0;
	BOOST_FOREACH(const CSGProduct &product, products.products) {
		std::vector<OpenCSG::Primitive*> primitives;
		if (!built) {
			BOOST_FOREACH(const CSGChainObject &csgobj, product.intersections) {
				if (csgobj.leaf->geom) primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Intersection, highlight_mode, background_mode, OPENSCAD_INTERSECTION, shaderinfo));
			}
			BOOST_FOREACH(const CSGChainObject &csgobj, product.subtractions) {
				if (csgobj.leaf->geom) primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Subtraction, highlight_mode, background_mode, OPENSCAD_DIFFERENCE, shaderinfo));
			}
			products_primitives->push_back(primitives);
		}
		if ((*products_primitives)[opencsg_ids_idx].size() > 1) {
			OpenCSG::render((*products_primitives)[opencsg_ids_idx]);
			glDepthFunc(GL_EQUAL);
		}
		opencsg_ids_idx++;
		if (shaderinfo) glUseProgram(shaderinfo[0]);

		const CSGChainObject &parent_obj = product.intersections[0];
		BOOST_FOREACH(const CSGChainObject &csgobj, product.intersections) {
			const Color4f &c = csgobj.leaf->color;
				csgmode_e csgmode = csgmode_e(
					highlight_mode ?
					CSGMODE_HIGHLIGHT :
					(background_mode ? CSGMODE_BACKGROUND : CSGMODE_NORMAL));

			ColorMode colormode = COLORMODE_NONE;
			if (highlight_mode) {
				colormode = COLORMODE_HIGHLIGHT;
			} else if (background_mode) {
				colormode = COLORMODE_BACKGROUND;
			} else {
				colormode = COLORMODE_MATERIAL;
			}
			setColor(colormode, c.data(), shaderinfo);
			if (!built) {
				products_ids->push_back(glGenLists(1));
				glNewList(products_ids->back(), GL_COMPILE);
				render_surface(csgobj.leaf->geom, csgmode, csgobj.leaf->matrix, shaderinfo);
				glEndList();
			}
			glPushMatrix();
			glMultMatrixd(csgobj.leaf->matrix.data());
			glCallList((*products_ids)[direct_ids_idx++]);
			glPopMatrix();
		}
		BOOST_FOREACH(const CSGChainObject &csgobj, product.subtractions) {
			const Color4f &c = csgobj.leaf->color;
				csgmode_e csgmode = csgmode_e(
					(highlight_mode ?
					CSGMODE_HIGHLIGHT :
					(background_mode ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) | CSGMODE_DIFFERENCE);

			ColorMode colormode = COLORMODE_NONE;
			if (highlight_mode) {
				colormode = COLORMODE_HIGHLIGHT;
			} else if (background_mode) {
				colormode = COLORMODE_BACKGROUND;
			} else {
				colormode = COLORMODE_CUTOUT;
			}

			setColor(colormode, c.data(), shaderinfo);
			if (!built) {
				products_ids->push_back(glGenLists(1));
				glNewList(products_ids->back(), GL_COMPILE);
				render_surface(csgobj.leaf->geom, csgmode, csgobj.leaf->matrix, shaderinfo);
				glEndList();
			}
			glPushMatrix();
			glMultMatrixd(csgobj.leaf->matrix.data());
			glCallList((*products_ids)[direct_ids_idx++]);
			glPopMatrix();
		}

		if (shaderinfo) glUseProgram(0);
		glDepthFunc(GL_LEQUAL);
	}

	if ((!highlight_mode && !background_mode) && !root_products_built_) {
		const_cast<OpenCSGRenderer *>(this)->root_products_built_ = true;
	}
	if ((highlight_mode && !background_mode) && !highlights_products_built_) {
		const_cast<OpenCSGRenderer *>(this)->highlights_products_built_ = true;
	}
	if ((!highlight_mode && background_mode) && !background_products_built_) {
		const_cast<OpenCSGRenderer *>(this)->background_products_built_ = true;
	}
#endif
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (root_products) bbox = root_products->getBoundingBox();
	if (highlights_products) bbox.extend(highlights_products->getBoundingBox());
	if (background_products) bbox.extend(background_products->getBoundingBox());

	return bbox;
}
