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

GeometryPrimitive::~GeometryPrimitive()
{
	if(id_ && built_) {
		glDeleteLists(id_, 1);
	}
}

void GeometryPrimitive::render()
{
	if (!built_) {
		id_ = glGenLists(1);
		glNewList(id_, GL_COMPILE);
			Renderer::render_surface(geom_, csgmode_, m_);
		glEndList();
		built_ = true;
	}

	glPushMatrix();
	glMultMatrixd(m_.data());
		glCallList(id_);
	glPopMatrix();
}
#endif

OpenCSGRenderer::OpenCSGRenderer(CSGChain *root_chain, CSGChain *highlights_chain, CSGChain *background_chain, GLint *shaderinfo)
	: root_chain_(root_chain), highlights_chain_(highlights_chain),
	  background_chain_(background_chain), shaderinfo_(shaderinfo),
	  root_chain_built_(false), highlights_chain_built_(false), background_chain_built_(false)
{
}

OpenCSGRenderer::~OpenCSGRenderer()
{
	if (root_chain_built_){
		for (size_t i = 0; i < root_chain_primitives_.size(); i++) {
			std::for_each(root_chain_primitives_[i].begin(), root_chain_primitives_[i].end(), del_fun<OpenCSG::Primitive>());
		}
		root_chain_primitives_.clear();
		for (size_t i = 0; i < root_chain_list_ids_.size(); i++) {
			glDeleteLists(root_chain_list_ids_[i],1);
		}
		root_chain_list_ids_.clear();
	}
	if (highlights_chain_built_){
		for (size_t i = 0; i < highlights_chain_primitives_.size(); i++) {
			std::for_each(highlights_chain_primitives_[i].begin(), highlights_chain_primitives_[i].end(), del_fun<OpenCSG::Primitive>());
		}
		highlights_chain_primitives_.clear();
		for (size_t i = 0; i < highlights_chain_list_ids_.size(); i++) {
			glDeleteLists(highlights_chain_list_ids_[i],1);
		}
		highlights_chain_list_ids_.clear();
	}
	if (background_chain_built_){
		for (size_t i = 0; i < background_chain_primitives_.size(); i++) {
			std::for_each(background_chain_primitives_[i].begin(), background_chain_primitives_[i].end(), del_fun<OpenCSG::Primitive>());
		}
		background_chain_primitives_.clear();
		for (size_t i = 0; i < background_chain_list_ids_.size(); i++) {
			glDeleteLists(background_chain_list_ids_[i],1);
		}
		background_chain_list_ids_.clear();
	}
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	GLint *shaderinfo = shaderinfo_;
	if (!shaderinfo[0]) shaderinfo = NULL;
	if (root_chain_) {
		renderCSGChain(root_chain_, showedges ? shaderinfo : NULL, false, false);
	}
	if (background_chain_) {
		renderCSGChain(background_chain_, showedges ? shaderinfo : NULL, false, true);
	}
	if (highlights_chain_) {
		renderCSGChain(highlights_chain_, showedges ? shaderinfo : NULL, true, false);
	}
}

void OpenCSGRenderer::renderCSGChain(CSGChain *chain, GLint *shaderinfo, bool highlight, bool background) const
{
#ifdef ENABLE_OPENCSG
	std::vector<OpenCSG::Primitive *> primitives;
	std::vector<std::vector<OpenCSG::Primitive *> > *chain_primitives;
	std::vector<unsigned int> *chain_lists;
	bool built;

	if (highlight && !background) {
		chain_primitives = const_cast<std::vector<std::vector<OpenCSG::Primitive *> > *>(&highlights_chain_primitives_);
		chain_lists = const_cast<std::vector<unsigned int>*>(&highlights_chain_list_ids_);
		built = highlights_chain_built_;
	} else if (!highlight && background) {
		chain_primitives = const_cast<std::vector<std::vector<OpenCSG::Primitive *> > *>(&background_chain_primitives_);
		chain_lists = const_cast<std::vector<unsigned int>*>(&background_chain_list_ids_);
		built = background_chain_built_;
	} else {
		chain_primitives = const_cast<std::vector<std::vector<OpenCSG::Primitive *> > *>(&root_chain_primitives_);
		chain_lists = const_cast<std::vector<unsigned int>*>(&root_chain_list_ids_);
		built = root_chain_built_;
	}

	size_t j = 0;
	size_t l = 0;
	size_t m = 0;
	for (size_t i = 0;; i++) {
		bool last = i == chain->objects.size();
		bool primitives_added = false;
		const CSGChainObject &i_obj = last ? chain->objects[i-1] : chain->objects[i];
		if ((last || i_obj.type == CSGTerm::TYPE_UNION) && (i != 0)) {
			if (j+1 != i) {
				if (!built) {
					chain_primitives->push_back(primitives);
					primitives_added = true;
				}
				OpenCSG::render((*chain_primitives)[l]);
				glDepthFunc(GL_EQUAL);
				l++;
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
					if (!built) {
						chain_lists->push_back(glGenLists(1));
						glNewList(chain_lists->back(), GL_COMPILE);
						render_surface(j_obj.geom, csgmode, j_obj.matrix, shaderinfo);
						glEndList();
					}

					glPushMatrix();
					glMultMatrixd(j_obj.matrix.data());

					setColor(colormode, c.data(), shaderinfo);
					glCallList((*chain_lists)[m]);
					glPopMatrix();

					m++;
				}
			}

			if (shaderinfo) glUseProgram(0);
			glDepthFunc(GL_LEQUAL);
			if (!built) {
				if (!primitives_added) {
					for_each(primitives.begin(),primitives.end(),del_fun<OpenCSG::Primitive>());
				}
				primitives.clear();
			}
		}

		if (last) break;

		if (i_obj.geom) {
			if (!built) {
				csgmode_e csgmode = csgmode_e((highlight ?
								CSGMODE_HIGHLIGHT :
								(background ? CSGMODE_BACKGROUND : CSGMODE_NORMAL)) |
								(i_obj.type == CSGTerm::TYPE_DIFFERENCE ? CSGMODE_DIFFERENCE : 0));

				GeometryPrimitive *prim = new GeometryPrimitive(i_obj.geom, i_obj.matrix, csgmode,
										i_obj.type == CSGTerm::TYPE_DIFFERENCE ?
										OpenCSG::Subtraction : OpenCSG::Intersection, i_obj.geom->getConvexity());

				primitives.push_back(prim);
			}
		}
	}

	if ((!highlight && !background) && !root_chain_built_) {
		const_cast<OpenCSGRenderer *>(this)->root_chain_built_ = true;
	}
	if ((highlight && !background) && !highlights_chain_built_) {
		const_cast<OpenCSGRenderer *>(this)->highlights_chain_built_ = true;
	}
	if ((!highlight && background) && !background_chain_built_) {
		const_cast<OpenCSGRenderer *>(this)->background_chain_built_ = true;
	}
#endif
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (root_chain_) bbox = root_chain_->getBoundingBox();
	if (background_chain_) bbox.extend(background_chain_->getBoundingBox());

	return bbox;
}
