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
	: root_products(root_products), highlight_products(highlight_products), background_products(background_products),
	  vertices_vbo(0), elements_vbo(0)
{
}

ThrownTogetherRenderer::~ThrownTogetherRenderer()
{
	if (vertices_vbo) {
		glDeleteBuffers(1, &vertices_vbo);
	}
	if (elements_vbo) {
		glDeleteBuffers(1, &elements_vbo);
	}
}

void ThrownTogetherRenderer::prepare(bool /*showfaces*/, bool showedges, const Renderer::shaderinfo_t *shaderinfo)
{
	PRINTD("Thrown prepare");
	if (Feature::ExperimentalVxORenderers.is_enabled() && !vertex_states.size()) {
		VertexArray vertex_array(std::make_shared<TTRVertexStateFactory>(), vertex_states);
		vertex_array.addSurfaceData();
		add_shader_data(vertex_array);
		
		if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			size_t vertices_size = 0, elements_size = 0;
			if (this->root_products)
				vertices_size += (getSurfaceBufferSize(this->root_products, false, false, true)*2);
			if (this->background_products)
				vertices_size += getSurfaceBufferSize(this->background_products, false, true, true);
			if (this->highlight_products)
				vertices_size += getSurfaceBufferSize(this->highlight_products, true, false, true);

			if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
				if (vertices_size <= 0xff) {
					vertex_array.addElementsData(std::make_shared<AttributeData<GLubyte,1,GL_UNSIGNED_BYTE>>());
				} else if (vertices_size <= 0xffff) {
					vertex_array.addElementsData(std::make_shared<AttributeData<GLushort,1,GL_UNSIGNED_SHORT>>());
				} else {
					vertex_array.addElementsData(std::make_shared<AttributeData<GLuint,1,GL_UNSIGNED_INT>>());
				}
				elements_size = vertices_size * vertex_array.elements().stride();
				vertex_array.elementsSize(elements_size);
			}
			vertices_size *= vertex_array.stride();
			vertex_array.verticesSize(vertices_size);

			GL_TRACE("glBindBuffer(GL_ARRAY_BUFFER, %d)", vertex_array.verticesVBO());
			glBindBuffer(GL_ARRAY_BUFFER, vertex_array.verticesVBO()); GL_ERROR_CHECK();
			GL_TRACE("glBufferData(GL_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", vertices_size % (void *)nullptr);
			glBufferData(GL_ARRAY_BUFFER, vertices_size, nullptr, GL_STATIC_DRAW); GL_ERROR_CHECK();
			if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
				GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d)", vertex_array.elementsVBO());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_array.elementsVBO()); GL_ERROR_CHECK();
				GL_TRACE("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_size % (void *)nullptr);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_size, nullptr, GL_STATIC_DRAW); GL_ERROR_CHECK();
			}
		} else if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
			vertex_array.addElementsData(std::make_shared<AttributeData<GLuint,1,GL_UNSIGNED_INT>>());
		}

		if (this->root_products)
			createCSGProducts(*this->root_products, vertex_array, false, false);
		if (this->background_products)
			createCSGProducts(*this->background_products, vertex_array, false, true);
		if (this->highlight_products)
			createCSGProducts(*this->highlight_products, vertex_array, true, false);
		
		if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
				GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
			}
			GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
			glBindBuffer(GL_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
		}

		vertex_array.createInterleavedVBOs();
		vertices_vbo = vertex_array.verticesVBO();
		elements_vbo = vertex_array.elementsVBO();
	}
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
	const PolySet* ps = dynamic_cast<const PolySet*>(csgobj.leaf->geom.get());
	if (!ps) return;

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
	render_surface(*ps, csgmode, m, shaderinfo);
	// only use old render_edges if there is no shader progid
	if (showedges && (shaderinfo && shaderinfo->progid == 0)) {
		// FIXME? glColor4f((c[0]+1)/2, (c[1]+1)/2, (c[2]+1)/2, 1.0);
		setColor(edge_colormode);
		render_edges(*ps, csgmode);
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
		for(const auto &vs : vertex_states) {
			if (vs) {
				std::shared_ptr<TTRVertexState> csg_vs = std::dynamic_pointer_cast<TTRVertexState>(vs);
				if (csg_vs) {
					if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
						GL_TRACE("glUniform3f(%d, %f, %f, %f)",
								shaderinfo->data.select_rendering.identifier %
								(((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f) %
								(((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f) %
								(((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f));
						glUniform3f(shaderinfo->data.select_rendering.identifier,
								((csg_vs->csgObjectIndex() >> 0) & 0xff) / 255.0f,
								((csg_vs->csgObjectIndex() >> 8) & 0xff) / 255.0f,
								((csg_vs->csgObjectIndex() >> 16) & 0xff) / 255.0f); GL_ERROR_CHECK();
					}
				}
				std::shared_ptr<VBOShaderVertexState> shader_vs = std::dynamic_pointer_cast<VBOShaderVertexState>(vs);
				if (!shader_vs || (shader_vs && showedges)) {
					vs->draw();
				}
			}
		}
	}
}

void ThrownTogetherRenderer::createChainObject(VertexArray &vertex_array,
                                               const class CSGChainObject &csgobj, bool highlight_mode,
                                               bool background_mode, OpenSCADOperator type)
{
	if (csgobj.leaf->geom) {
		const PolySet* ps = dynamic_cast<const PolySet*>(csgobj.leaf->geom.get());
		if (!ps) return;

		if (this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return;

		Color4f color;
		Color4f &leaf_color = csgobj.leaf->color;
		csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);

		vertex_array.writeSurface();
		add_shader_pointers(vertex_array);

		if (highlight_mode || background_mode) {
			const ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
			getShaderColor(colormode, leaf_color, color);

			shaderinfo_t shader_info = this->getShader();
			std::shared_ptr<VertexState> color_state = std::make_shared<VBOShaderVertexState>(0,0,vertex_array.verticesVBO(),vertex_array.elementsVBO());
			color_state->glBegin().emplace_back([shader_info,color]() {
				GL_TRACE("glUniform4f(%d, %f, %f, %f, %f)", shader_info.data.csg_rendering.color_area % color[0] % color[1] % color[2] % color[3]);
				glUniform4f(shader_info.data.csg_rendering.color_area, color[0], color[1], color[2], color[3]); GL_ERROR_CHECK();
				GL_TRACE("glUniform4f(%d, %f, %f, %f, 1.0)", shader_info.data.csg_rendering.color_edge % (color[0]+1)/2 % (color[1]+1)/2 % (color[2]+1)/2);
				glUniform4f(shader_info.data.csg_rendering.color_edge, (color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0); GL_ERROR_CHECK();
			});
			vertex_states.emplace_back(std::move(color_state));
			
			create_surface(*ps, vertex_array, csgmode, csgobj.leaf->matrix, color);
			std::shared_ptr<TTRVertexState> vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back());
			if (vs) {
				vs->csgObjectIndex(csgobj.leaf->index);
			}
		} else { // root mode
			ColorMode colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, false, type);
			getShaderColor(colormode, leaf_color, color);

			shaderinfo_t shader_info = this->getShader();
			std::shared_ptr<VertexState> color_state = std::make_shared<VBOShaderVertexState>(0,0,vertex_array.verticesVBO(),vertex_array.elementsVBO());
			color_state->glBegin().emplace_back([shader_info,color]() {
				GL_TRACE("glUniform4f(%d, %f, %f, %f, %f)", shader_info.data.csg_rendering.color_area % color[0] % color[1] % color[2] % color[3]);
				glUniform4f(shader_info.data.csg_rendering.color_area, color[0], color[1], color[2], color[3]); GL_ERROR_CHECK();
				GL_TRACE("glUniform4f(%d, %f, %f, %f, 1.0)", shader_info.data.csg_rendering.color_edge % (color[0]+1)/2 % (color[1]+1)/2 % (color[2]+1)/2);
				glUniform4f(shader_info.data.csg_rendering.color_edge, (color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0); GL_ERROR_CHECK();
			});
			vertex_states.emplace_back(std::move(color_state));
			
			std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
			cull->glBegin().emplace_back([]() { GL_TRACE0("glEnable(GL_CULL_FACE)"); glEnable(GL_CULL_FACE); GL_ERROR_CHECK(); });
			cull->glBegin().emplace_back([]() { GL_TRACE0("glCullFace(GL_BACK)"); glCullFace(GL_BACK); GL_ERROR_CHECK(); });
			vertex_states.emplace_back(std::move(cull));

			create_surface(*ps, vertex_array, csgmode, csgobj.leaf->matrix, color);
			std::shared_ptr<TTRVertexState> vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back());
			if (vs) {
				vs->csgObjectIndex(csgobj.leaf->index);
			}

			color[0] = 1.0; color[1] = 0.0; color[2] = 1.0; // override leaf color on front/back error

			colormode = getColorMode(csgobj.flags, highlight_mode, background_mode, true, type);
			getShaderColor(colormode, leaf_color, color);

			shader_info = this->getShader();
			color_state = std::make_shared<VBOShaderVertexState>(0,0,vertex_array.verticesVBO(),vertex_array.elementsVBO());
			color_state->glBegin().emplace_back([shader_info,color]() {
				GL_TRACE("glUniform4f(%d, %f, %f, %f, %f)", shader_info.data.csg_rendering.color_area % color[0] % color[1] % color[2] % color[3]);
				glUniform4f(shader_info.data.csg_rendering.color_area, color[0], color[1], color[2], color[3]); GL_ERROR_CHECK();
				GL_TRACE("glUniform4f(%d, %f, %f, %f, 1.0)", shader_info.data.csg_rendering.color_edge % (color[0]+1)/2 % (color[1]+1)/2 % (color[2]+1)/2);
				glUniform4f(shader_info.data.csg_rendering.color_edge, (color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0); GL_ERROR_CHECK();
			});
			vertex_states.emplace_back(std::move(color_state));

			cull = std::make_shared<VertexState>();
			cull->glBegin().emplace_back([]() { GL_TRACE0("glCullFace(GL_FRONT)"); glCullFace(GL_FRONT); GL_ERROR_CHECK(); });
			vertex_states.emplace_back(std::move(cull));

			create_surface(*ps, vertex_array, csgmode, csgobj.leaf->matrix, color);
			vs = std::dynamic_pointer_cast<TTRVertexState>(vertex_array.states().back());
			if (vs) {
				vs->csgObjectIndex(csgobj.leaf->index);
			}
			
			vertex_states.back()->glEnd().emplace_back([]() { GL_TRACE0("glDisable(GL_CULL_FACE)"); glDisable(GL_CULL_FACE); GL_ERROR_CHECK(); });
		}
	}
}

void ThrownTogetherRenderer::createCSGProducts(const CSGProducts &products, VertexArray &vertex_array,
						bool highlight_mode, bool background_mode)
{
	PRINTD("Thrown renderCSGProducts");
	this->geomVisitMark.clear();

	for(const auto &product : products.products) {
		for(const auto &csgobj : product.intersections) {
			createChainObject(vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION);
		}
		for(const auto &csgobj : product.subtractions) {
			createChainObject(vertex_array, csgobj, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);
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
