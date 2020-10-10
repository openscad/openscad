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
#include "feature.h"
#include "polyset.h"
#include "printutils.h"

#include "system-gl.h"

ThrownTogetherRenderer::ThrownTogetherRenderer(shared_ptr<CSGProducts> root_products,
						shared_ptr<CSGProducts> highlight_products,
						shared_ptr<CSGProducts> background_products)
	: root_products(root_products), highlight_products(highlight_products), background_products(background_products)
{
}

ThrownTogetherRenderer::~ThrownTogetherRenderer()
{
	glDeleteBuffers(1,&vbo);
}

void ThrownTogetherRenderer::draw(bool /*showfaces*/, bool showedges, const Renderer::shaderinfo_t *shaderinfo) const
{
	PRINTD("Thrown draw");
	if (!shaderinfo && showedges) {
		shaderinfo = &getShader();
	}
	if (shaderinfo && shaderinfo->progid) {
		glUseProgram(shaderinfo->progid);
		if (shaderinfo->type == EDGE_RENDERING && showedges) {
			glUniform1f(shaderinfo->data.csg_rendering.xscale, shaderinfo->vp_size_x);
			glUniform1f(shaderinfo->data.csg_rendering.yscale, shaderinfo->vp_size_y);
			shader_attribs_enable();
		}
	}

	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
	 	if (this->root_products) {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			renderCSGProducts(this->root_products, showedges, shaderinfo, false, false, false);
			glCullFace(GL_FRONT);
			glColor3ub(255, 0, 255);
			renderCSGProducts(this->root_products, showedges, shaderinfo, false, false, true);
			glDisable(GL_CULL_FACE);
		}
		if (this->background_products)
		 	renderCSGProducts(this->background_products, showedges, shaderinfo, false, true, false);
		if (this->highlight_products)
		 	renderCSGProducts(this->highlight_products, showedges, shaderinfo, true, false, false);
	} else {
		if (!vertex_states.size()) {
			VertexArray vertex_array(std::make_unique<TTRVertexStateFactory>(), vertex_states);
			vertex_array.addSurfaceData();
			add_shader_data(vertex_array);

			if (this->root_products)
				createCSGProducts(*this->root_products, vertex_array, false, false);
			if (this->background_products)
				createCSGProducts(*this->background_products, vertex_array, false, true);
			if (this->highlight_products)
				createCSGProducts(*this->highlight_products, vertex_array, true, false);
				
			glGenBuffers(1, &vbo);
			vertex_array.createInterleavedVBO(vbo);
		}

		renderCSGProducts(std::make_shared<CSGProducts>(), showedges, shaderinfo);
		
	}
	if (shaderinfo && shaderinfo->progid) {
		if (shaderinfo->type == EDGE_RENDERING && showedges) {
			shader_attribs_disable();
		}
		glUseProgram(0);
	}
}

void ThrownTogetherRenderer::renderChainObject(const CSGChainObject &csgobj, bool showedges,
						const Renderer::shaderinfo_t *shaderinfo,
						bool highlight_mode, bool background_mode,
						bool fberror, OpenSCADOperator type) const
{
	if (this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return;
	const Color4f &c = csgobj.leaf->color;
	csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);
	ColorMode colormode = ColorMode::NONE;
	ColorMode edge_colormode = ColorMode::NONE;

	colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, fberror, type);
	const Transform3d &m = csgobj.leaf->matrix;

	if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
		int identifier = csgobj.leaf->index;
		glUniform3f(shaderinfo->data.select_rendering.identifier, ((identifier >> 0) & 0xff) / 255.0f,
								((identifier >> 8) & 0xff) / 255.0f, ((identifier >> 16) & 0xff) / 255.0f);
	}
	else {
		setColor(colormode, c.data());
	}
	glPushMatrix();
	glMultMatrixd(m.data());
	render_surface(csgobj.leaf->geom, csgmode, m, shaderinfo);
	// only use old render_edges if there is no shader progid
	if (showedges && (shaderinfo && shaderinfo->progid == 0)) {
		// FIXME? glColor4f((c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
		setColor(edge_colormode);
		render_edges(csgobj.leaf->geom, csgmode);
	}
	glPopMatrix();

}

void ThrownTogetherRenderer::renderCSGProducts(const std::shared_ptr<CSGProducts> &products, bool showedges,
						const Renderer::shaderinfo_t *shaderinfo,
						bool highlight_mode, bool background_mode,
						bool fberror) const
{
	PRINTD("Thrown renderCSGProducts");
	glDepthFunc(GL_LEQUAL);
	this->geomVisitMark.clear();

	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		for(const auto &product : products->products) {
			for(const auto &csgobj : product.intersections) {
				renderChainObject(csgobj, showedges, shaderinfo, highlight_mode, background_mode, fberror, OpenSCADOperator::INTERSECTION);
			}
			for(const auto &csgobj : product.subtractions) {
				renderChainObject(csgobj, showedges, shaderinfo, highlight_mode, background_mode, fberror, OpenSCADOperator::DIFFERENCE);
			}
		}
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		for(const auto &vertex : vertex_states) {
			if (vertex) {
				std::shared_ptr<TTRVertexState> csg_vertex = std::dynamic_pointer_cast<TTRVertexState>(vertex);
				if (csg_vertex) {
					if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
						glUniform3f(shaderinfo->data.select_rendering.identifier,
								((csg_vertex->csgObjectIndex() >> 0) & 0xff) / 255.0f,
								((csg_vertex->csgObjectIndex() >> 8) & 0xff) / 255.0f,
								((csg_vertex->csgObjectIndex() >> 16) & 0xff) / 255.0f);
					}
				}
				std::shared_ptr<VBOShaderVertexState> shader_vertex = std::dynamic_pointer_cast<VBOShaderVertexState>(vertex);
				if (!shader_vertex || (shader_vertex && showedges)) {
					vertex->drawArrays();
				}
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void ThrownTogetherRenderer::createChainObject(VertexArray &vertex_array,
                                               const class CSGChainObject &csgobj, bool highlight_mode,
                                               bool background_mode, OpenSCADOperator type) const
{
	Color4f color;

	if (csgobj.leaf->geom) {
		if (this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return;

		Color4f &leaf_color = csgobj.leaf->color;
		csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);

		vertex_array.writeSurface();
		add_shader_pointers(vertex_array);

		if (highlight_mode || background_mode) {
			const ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
			getShaderColor(colormode, leaf_color, color);
			
			create_surface(csgobj.leaf->geom.get(), vertex_array, csgmode, csgobj.leaf->matrix, color);
			std::shared_ptr<TTRVertexState> vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back());
			if (vs) {
				vs->csgObjectIndex(csgobj.leaf->index);
			}
		} else { // root mode
			ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
			getShaderColor(colormode, leaf_color, color);
			
			std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
			cull->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glEnable(GL_CULL_FACE)"); glEnable(GL_CULL_FACE); });
			cull->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_BACK)"); glCullFace(GL_BACK); });
			vertex_states.emplace_back(std::move(cull));

			create_surface(csgobj.leaf->geom.get(), vertex_array, csgmode, csgobj.leaf->matrix, color);
			std::shared_ptr<TTRVertexState> vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back());
			if (vs) {
				vs->csgObjectIndex(csgobj.leaf->index);
			}

			color[0] = 1.0; color[1] = 0.0; color[2] = 1.0; // override leaf color on front/back error

			colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, true, type);
			getShaderColor(colormode, leaf_color, color);

			cull = std::make_shared<VertexState>();
			cull->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_FRONT)"); glCullFace(GL_FRONT); });
			vertex_states.emplace_back(std::move(cull));

			create_surface(csgobj.leaf->geom.get(), vertex_array, csgmode, csgobj.leaf->matrix, color);
			vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back());
			if (vs) {
				vs->csgObjectIndex(csgobj.leaf->index);
			}
			
			vertex_states.back()->glEnd().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisable(GL_CULL_FACE)"); glDisable(GL_CULL_FACE); });
		}
	}
}

void ThrownTogetherRenderer::createCSGProducts(const CSGProducts &products, VertexArray &vertex_array,
						bool highlight_mode, bool background_mode) const
{
	PRINTD("Thrown renderCSGProducts");
	this->geomVisitMark.clear();

	for(const auto &product : products.products) {
		if (product.intersections.size() || product.subtractions.size()) {
			for(const auto &csgobj : product.intersections) {
				createChainObject(vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION);
			}
			for(const auto &csgobj : product.subtractions) {
				createChainObject(vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);
			}
		}
	}
}

Renderer::ColorMode ThrownTogetherRenderer::getColorMode(const CSGNode::Flag &flags, bool highlight_mode,
							 bool background_mode, bool fberror, OpenSCADOperator type) const
{
	ColorMode colormode = ColorMode::NONE;

	if (highlight_mode) {
		colormode = ColorMode::HIGHLIGHT;
	} else if (background_mode) {
		if (flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::BACKGROUND;
		}
	} else if (fberror) {
	} else if (type == OpenSCADOperator::DIFFERENCE) {
		if (flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::CUTOUT;
		}
	} else {
		if (flags & CSGNode::FLAG_HIGHLIGHT) {
			colormode = ColorMode::HIGHLIGHT;
		}
		else {
			colormode = ColorMode::MATERIAL;
		}
	}
	return colormode;
}

BoundingBox ThrownTogetherRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_products) bbox = this->root_products->getBoundingBox();
	if (this->highlight_products) bbox.extend(this->highlight_products->getBoundingBox());
	if (this->background_products) bbox.extend(this->background_products->getBoundingBox());
	return bbox;
}
