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
#include "feature.h"

#define OPENGL_TEST(place) \
do { \
	auto err = glGetError(); \
	if (err != GL_NO_ERROR) { \
		fprintf(stderr, "OpenGL error " place ":\n %s\n\n", gluErrorString(err)); \
	} \
} while (false)

#ifdef ENABLE_OPENCSG
#include <opencsg.h>

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity, const OpenCSGRenderer &renderer) :
			OpenCSG::Primitive(operation, convexity), csgmode(Renderer::CSGMODE_NONE), renderer(renderer) { }
	shared_ptr<const Geometry> geom;
	Transform3d m;
	Renderer::csgmode_e csgmode;

	void render() override {
		glPushMatrix();
		glMultMatrixd(m.data());
		renderer.render_surface(geom, csgmode, m);
		glPopMatrix();
	}
private:
	const OpenCSGRenderer &renderer;
};

class OpenCSGVBOPrim : public OpenCSG::Primitive
{
public:
	OpenCSGVBOPrim(OpenCSG::Operation operation, unsigned int convexity, const OpenCSGRenderer &renderer, const VBORenderer::VertexSet &vertex_set, const GLuint vbo) :
			OpenCSG::Primitive(operation, convexity), renderer(renderer), vertex_set(vertex_set), vbo(vbo) { }
	virtual void render() {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		renderer.draw_surface(vertex_set);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

private:
	const OpenCSGRenderer &renderer;
	const OpenCSGRenderer::VertexSet &vertex_set;
	const GLuint vbo;
};
#endif // ENABLE_OPENCSG

OpenCSGRenderer::OpenCSGRenderer(shared_ptr<CSGProducts> root_products,
				 shared_ptr<CSGProducts> highlights_products,
				 shared_ptr<CSGProducts> background_products)
	: product_vertex_sets(std::make_shared<ProductVertexSets>()),
	  root_products(root_products),
	  highlights_products(highlights_products),
	  background_products(background_products)
{
}

OpenCSGRenderer::~OpenCSGRenderer()
{
	if (product_vertex_sets) {
		for (auto &vertex_sets : *product_vertex_sets) {
			glDeleteBuffers(1,&vertex_sets.first);
			vertex_sets.second->clear();
		}
		product_vertex_sets->clear();
	}
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	const Renderer::shaderinfo_t *shaderinfo = &getShader();
	bool use_shader = true;
	if (!shaderinfo->progid) use_shader = false;
	if (!showedges) use_shader = false;

	this->draw_with_shader(use_shader ? shaderinfo : nullptr);
}

void OpenCSGRenderer::draw_with_shader(const Renderer::shaderinfo_t *shaderinfo) const
{
	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		if (this->root_products) {
			renderCSGProducts(this->root_products, shaderinfo, false, false);
		}
		if (this->background_products) {
			renderCSGProducts(this->background_products, shaderinfo, false, true);
		}
		if (this->highlights_products) {
			renderCSGProducts(this->highlights_products, shaderinfo, true, false);
		}
	} else {
		PRINTD("OpenCSGRenderer VBO");
		if (!getProductVertexSets()->size()) {
			if (this->root_products) {
				createCSGProducts(*this->root_products, false, false);
			}
			if (this->background_products) {
				createCSGProducts(*this->background_products, false, true);
			}
			if (this->highlights_products) {
				createCSGProducts(*this->highlights_products, true, false);
			}
		}
		renderCSGProducts(std::make_shared<CSGProducts>(), shaderinfo);
	}
}

// Primitive for rendering using OpenCSG
OpenCSGPrim *OpenCSGRenderer::createCSGPrimitive(const CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type) const
{
	OpenCSGPrim *prim = new OpenCSGPrim(operation, csgobj.leaf->geom->getConvexity(), *this);
	prim->geom = csgobj.leaf->geom;
	prim->m = csgobj.leaf->matrix;
	prim->csgmode = get_csgmode(highlight_mode, background_mode, type);
	return prim;
}

// Primitive for drawing using OpenCSG
OpenCSGVBOPrim *OpenCSGRenderer::createVBOPrimitive(const VertexSet &vertex_set, const GLuint vbo) const
{
	if (vertex_set.operation == OpenSCADOperator::INTERSECTION)
		return new OpenCSGVBOPrim(OpenCSG::Intersection, vertex_set.convexity, *this, vertex_set, vbo);

	if (vertex_set.operation == OpenSCADOperator::DIFFERENCE)
		return new OpenCSGVBOPrim(OpenCSG::Subtraction, vertex_set.convexity, *this, vertex_set, vbo);

	return nullptr;
}

void OpenCSGRenderer::createCSGProducts(const CSGProducts &products, bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
	std::unique_ptr<std::vector<Vertex>> render_buffer = std::make_unique<std::vector<Vertex>>();
	std::unique_ptr<VertexSets> vertex_sets;
	VertexSet *prev = nullptr;
	VertexSet *vertex_set = nullptr;

	GLuint vbo;

	for(const auto &product : products.products) {
		// one vbo for each product
		if (product.intersections.size() || product.subtractions.size()) {
			vertex_sets = std::make_unique<std::vector<std::unique_ptr<VertexSet>>>();

			GLsizei combined_draw_size = 0;
			Color4f last_color;

			for(const auto &csgobj : product.intersections) {
				if (csgobj.leaf->geom && vertex_sets) {
					prev = (vertex_sets->empty() ? nullptr : vertex_sets->back().get());
					vertex_set = new VertexSet({SHADER, true, OpenSCADOperator::INTERSECTION, csgobj.leaf->geom->getConvexity(), GL_TRIANGLES, 0, 0, false, false, false, false, false, csgobj.leaf->index});
					GLintptr prev_start_offset = 0;
					GLsizei prev_draw_size = 0;
					if (prev) {
						prev_start_offset = prev->start_offset;
						prev_draw_size = prev->draw_size;
					}

					const Color4f &c = csgobj.leaf->color;
					csgmode_e csgmode = get_csgmode(highlight_mode, background_mode);

					ColorMode colormode = ColorMode::NONE;
					if (highlight_mode) {
						colormode = ColorMode::HIGHLIGHT;
					} else if (background_mode) {
						colormode = ColorMode::BACKGROUND;
					} else {
						colormode = ColorMode::MATERIAL;
					}

					Color4f c1; // = setColor(colormode, c.data(), shaderinfo);
					if (getShaderColor(colormode, c, c1)) {
						last_color = c1;
					}

					this->create_surface(csgobj.leaf->geom, *render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
								csgmode, csgobj.leaf->matrix, last_color);

					if (c1[3] == 1.0f) {
						// object is opaque, draw normally
						vertex_set->draw_cull_front = false;
						vertex_set->draw_cull_back = false;
					} else {
						// object is transparent, so draw rear faces first.  Issue #1496
						vertex_set->draw_cull_front = true;
						vertex_set->draw_cull_back = true;
					}

					vertex_sets->push_back(std::unique_ptr<VertexSet>(vertex_set));
					combined_draw_size += vertex_set->draw_size;
				}
			}

			for(const auto &csgobj : product.subtractions) {
				if (csgobj.leaf->geom && vertex_sets) {
					prev = (vertex_sets->empty() ? 0 : vertex_sets->back().get());
					vertex_set = new VertexSet({SHADER, true, OpenSCADOperator::DIFFERENCE, csgobj.leaf->geom->getConvexity(), GL_TRIANGLES, 0, 0, false, false, false, false, false, csgobj.leaf->index});
					GLintptr prev_start_offset = 0;
					GLsizei prev_draw_size = 0;
					if (prev) {
						prev_start_offset = prev->start_offset;
						prev_draw_size = prev->draw_size;
					}

					const Color4f &c = csgobj.leaf->color;
					csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);

					ColorMode colormode = ColorMode::NONE;
					if (highlight_mode) {
						colormode = ColorMode::HIGHLIGHT;
					} else if (background_mode) {
						colormode = ColorMode::BACKGROUND;
					} else {
						colormode = ColorMode::CUTOUT;
					}

					Color4f c1; // = setColor(colormode, c.data(), shaderinfo);
					if (getShaderColor(colormode, c, c1)) {
						last_color = c1;
					}
					// negative objects should only render rear faces
					this->create_surface(csgobj.leaf->geom, *render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
								csgmode, csgobj.leaf->matrix, last_color);
					vertex_set->draw_cull_front = true;
					vertex_set->draw_cull_back = false;
					vertex_sets->push_back(std::unique_ptr<VertexSet>(vertex_set));
					combined_draw_size += vertex_set->draw_size;
				}
			}

			if (render_buffer->size()) {
				glGenBuffers(1, &vbo);

				product_vertex_sets->emplace_back(vbo,std::move(vertex_sets));

				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, render_buffer->size()*sizeof(Vertex), render_buffer->data(), GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				render_buffer->clear();
			}
		}
	}
#endif // ENABLE_OPENCSG
}

void OpenCSGRenderer::renderCSGProducts(const shared_ptr<CSGProducts> &products, const Renderer::shaderinfo_t *shaderinfo, bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		for(const auto &product : products->products) {
			std::vector<OpenCSG::Primitive*> primitives;
			for(const auto &csgobj : product.intersections) {
				if (csgobj.leaf->geom) primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Intersection, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION));
			}
			for(const auto &csgobj : product.subtractions) {
				if (csgobj.leaf->geom) primitives.push_back(createCSGPrimitive(csgobj, OpenCSG::Subtraction, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE));
			}
			if (primitives.size() > 1) {
				OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			OPENGL_TEST("start");
			if (shaderinfo && shaderinfo->progid) glUseProgram(shaderinfo->progid);
			OPENGL_TEST("load shader");

			for (const auto &csgobj : product.intersections) {
				if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
					int identifier = csgobj.leaf->index;
					glUniform3f(shaderinfo->data.select_rendering.identifier,
							((identifier >> 0) & 0xff) / 255.0f, ((identifier >> 8) & 0xff) / 255.0f,
							((identifier >> 16) & 0xff) / 255.0f);
					OPENGL_TEST("setup shader");
				}

				const Color4f &c = csgobj.leaf->color;
				csgmode_e csgmode = get_csgmode(highlight_mode, background_mode);

				ColorMode colormode = ColorMode::NONE;
				if (highlight_mode) {
					colormode = ColorMode::HIGHLIGHT;
				} else if (background_mode) {
					colormode = ColorMode::BACKGROUND;
				} else {
					colormode = ColorMode::MATERIAL;
				}

				glPushMatrix();
				glMultMatrixd(csgobj.leaf->matrix.data());

				const Color4f c1 = setColor(colormode, c.data(), shaderinfo);
				if (c1[3] == 1.0f) {
					// object is opaque, draw normally
					render_surface(csgobj.leaf->geom, csgmode, csgobj.leaf->matrix, shaderinfo);
				} else {
					// object is transparent, so draw rear faces first.  Issue #1496
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);
					render_surface(csgobj.leaf->geom, csgmode, csgobj.leaf->matrix, shaderinfo);
					glCullFace(GL_BACK);
					render_surface(csgobj.leaf->geom, csgmode, csgobj.leaf->matrix, shaderinfo);
					glDisable(GL_CULL_FACE);
				}
				OPENGL_TEST("render");

				glPopMatrix();
			}
			for(const auto &csgobj : product.subtractions) {
				const Color4f &c = csgobj.leaf->color;
				csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);

				ColorMode colormode = ColorMode::NONE;
				if (highlight_mode) {
					colormode = ColorMode::HIGHLIGHT;
				} else if (background_mode) {
					colormode = ColorMode::BACKGROUND;
				} else {
					colormode = ColorMode::CUTOUT;
				}

				setColor(colormode, c.data(), shaderinfo);
				glPushMatrix();
				glMultMatrixd(csgobj.leaf->matrix.data());
				// negative objects should only render rear faces
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				render_surface(csgobj.leaf->geom, csgmode, csgobj.leaf->matrix, shaderinfo);
				glDisable(GL_CULL_FACE);
				glPopMatrix();
			}

			if (shaderinfo) glUseProgram(0);
			for(auto &p : primitives) delete p;
			glDepthFunc(GL_LEQUAL);
		}
	} else {
		PRINTD("OpenCSGRenderer::renderCSGProducts");
		for(const auto &vertex_sets : *product_vertex_sets) {
			std::vector<OpenCSG::Primitive*> primitives;

			for(const auto &vertex_set : *vertex_sets.second) {
				if (vertex_set->is_opencsg_vertex_set) {
					OpenCSG::Primitive *primitive = nullptr;
					if ((primitive = createVBOPrimitive(*vertex_set, vertex_sets.first)) != nullptr) {
						primitives.push_back(primitive);
					}
				}
			}

			if (primitives.size() > 1) {
				OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			if (shaderinfo && shaderinfo->progid) glUseProgram(shaderinfo->progid);

			glBindBuffer(GL_ARRAY_BUFFER, vertex_sets.first);
			for(const auto &vertex_set : *vertex_sets.second) {
				if (vertex_set->is_opencsg_vertex_set)
				{
					if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
						glUniform3f(shaderinfo->data.select_rendering.identifier,
								((vertex_set->identifier >> 0) & 0xff) / 255.0f,
								((vertex_set->identifier >> 8) & 0xff) / 255.0f, ((vertex_set->identifier >> 16) & 0xff) / 255.0f);
					}
										
					if (!vertex_set->draw_cull_front && !vertex_set->draw_cull_back) {
						draw_surface(*vertex_set, shaderinfo, true);
					}
					if (vertex_set->draw_cull_front) {
						glEnable(GL_CULL_FACE); // cull face
						glCullFace(GL_FRONT);
						draw_surface(*vertex_set, shaderinfo, true);
						glDisable(GL_CULL_FACE);
					}
					if (vertex_set->draw_cull_back) {
						glEnable(GL_CULL_FACE); // cull face
						glCullFace(GL_BACK);
						draw_surface(*vertex_set, shaderinfo, true);
						glDisable(GL_CULL_FACE);
					}
				}
			}

			glBindBuffer(GL_ARRAY_BUFFER,0);

			if (shaderinfo) glUseProgram(0);
			for(auto &p : primitives) delete p;
			glDepthFunc(GL_LEQUAL);
		}
	}

#endif // ENABLE_OPENCSG
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_products) bbox = this->root_products->getBoundingBox();
	if (this->highlights_products) bbox.extend(this->highlights_products->getBoundingBox());
	if (this->background_products) bbox.extend(this->background_products->getBoundingBox());


	return bbox;
}
