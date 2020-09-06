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
	OpenCSGVBOPrim(OpenCSG::Operation operation, unsigned int convexity, std::unique_ptr<VertexState> vertex_state, const GLuint vbo) :
			OpenCSG::Primitive(operation, convexity), vertex_state(std::move(vertex_state)), vbo(vbo) { }
	virtual void render() {
		if (vertex_state != nullptr && vbo != 0) {
			if (OpenSCAD::debug != "") PRINTD("OpenCSGVBOPrim draw");
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			vertex_state->drawArrays();
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		} else {
			if (OpenSCAD::debug != "") PRINTD("OpenCSGVBOPrim vertex_state was null");
		}
	}

private:
	const std::unique_ptr<VertexState> vertex_state;
	const GLuint vbo;
};
#endif // ENABLE_OPENCSG

OpenCSGRenderer::OpenCSGRenderer(shared_ptr<CSGProducts> root_products,
				 shared_ptr<CSGProducts> highlights_products,
				 shared_ptr<CSGProducts> background_products)
	: root_products(root_products),
	  highlights_products(highlights_products),
	  background_products(background_products)
{
}

OpenCSGRenderer::~OpenCSGRenderer()
{
	for (auto &product : this->vbo_vertex_products) {
		GLuint vbo = product->vbo();
		glDeleteBuffers(1, &vbo);
	}
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges, const shaderinfo_t *shaderinfo) const
{
	PRINTDB("shaderinfo : %p", shaderinfo);
	if (!shaderinfo) shaderinfo = &getShader();

	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		if (this->root_products) {
			renderCSGProducts(this->root_products, showedges, shaderinfo, false, false);
		}
		if (this->background_products) {
			renderCSGProducts(this->background_products, showedges, shaderinfo, false, true);
		}
		if (this->highlights_products) {
			renderCSGProducts(this->highlights_products, showedges, shaderinfo, true, false);
		}
	} else {
		if (!vbo_vertex_products.size()) {
			if (this->root_products) {
				createCSGProducts(*this->root_products, shaderinfo, false, false);
			}
			if (this->background_products) {
				createCSGProducts(*this->background_products, shaderinfo, false, true);
			}
			if (this->highlights_products) {
				createCSGProducts(*this->highlights_products, shaderinfo, true, false);
			}
		}
		renderCSGProducts(std::make_shared<CSGProducts>(), showedges, shaderinfo);
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
OpenCSGVBOPrim *OpenCSGRenderer::createVBOPrimitive(const std::shared_ptr<OpenCSGVertexState> vertex_state,
						    const OpenCSG::Operation operation, const unsigned int convexity, const GLuint vbo) const
{
	std::unique_ptr<VertexState> opencsg_vs = std::make_unique<VertexState>(vertex_state->drawType(), vertex_state->drawSize());
	// First two glBegin entries are the vertex position calls
	opencsg_vs->glBegin().insert(opencsg_vs->glBegin().begin(), vertex_state->glBegin().begin(), vertex_state->glBegin().begin()+2);
	// First glEnd entry is the disable vertex position call
	opencsg_vs->glEnd().insert(opencsg_vs->glEnd().begin(), vertex_state->glEnd().begin(), vertex_state->glEnd().begin()+1);

	PRINTDB("createVBOPrimitive: %s, %d, %d", (
		(operation == OpenCSG::Intersection) ? "OpenCSG::Intersection" :
		(operation == OpenCSG::Subtraction) ? "OpenCSG::Subtraction" : "UNDEFINED"
	) % convexity % vbo);
	return new OpenCSGVBOPrim(operation, convexity, std::move(opencsg_vs), vbo);
}

void OpenCSGRenderer::createCSGProducts(const CSGProducts &products, const Renderer::shaderinfo_t *shaderinfo, bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
	for(const auto &product : products.products) {
		Color4f last_color;
		GLuint vertex_vbo = 0;
		std::unique_ptr<VertexStates> vertex_states = std::make_unique<VertexStates>();
		std::unique_ptr<OpenCSGPrimitives> primitives = std::make_unique<OpenCSGPrimitives>();
		VertexArray vertex_array(std::make_unique<OpenCSGVertexStateFactory>(), *(vertex_states.get()));
		std::shared_ptr<VertexData> vertex_data = std::make_shared<VertexData>();
		vertex_data->addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
		vertex_data->addNormalData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
		vertex_data->addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());
		vertex_array.addVertexData(vertex_data);
		
		glGenBuffers(1, &vertex_vbo);

		for(const auto &csgobj : product.intersections) {
			if (csgobj.leaf->geom) {
				vertex_array.writeIndex(0);
				PRINTD("OpenCSG product intersections");
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
				
				PRINTDB("colormode = %s",
					((colormode == ColorMode::NONE) ? "ColorMode::NONE" :
					(colormode == ColorMode::HIGHLIGHT) ? "ColorMode::HIGHLIGHT" :
					(colormode == ColorMode::BACKGROUND) ? "ColorMode::BACKGROUND" :
					(colormode == ColorMode::MATERIAL) ? "ColorMode::MATERIAL" : "UNKNOWN")
				);

				Color4f color; // = setColor(colormode, c.data(), shaderinfo);
				if (getShaderColor(colormode, c, color)) {
					last_color = color;
				}

				if (color[3] == 1.0f) {
					// object is opaque, draw normally
					this->create_surface(csgobj.leaf->geom, vertex_array, csgmode, csgobj.leaf->matrix, last_color);
					std::shared_ptr<OpenCSGVertexState> surface = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_states->back());
					if (surface != nullptr) {
						surface->csgObjectIndex(csgobj.leaf->index);
						primitives->emplace_back(std::move(createVBOPrimitive(surface,
												      OpenCSG::Intersection,
												      csgobj.leaf->geom->getConvexity(), vertex_vbo)));
					}
				} else {
					// object is transparent, so draw rear faces first.  Issue #1496
					std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
					cull->glBegin().emplace_back([]() { PRINTD("glEnable(GL_CULL_FACE)"); glEnable(GL_CULL_FACE); });
					cull->glBegin().emplace_back([]() { PRINTD("glCullFace(GL_FRONT)"); glCullFace(GL_FRONT); });
					vertex_states->emplace_back(std::move(cull));

					this->create_surface(csgobj.leaf->geom, vertex_array, csgmode, csgobj.leaf->matrix, last_color);
					std::shared_ptr<OpenCSGVertexState> surface = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_states->back());
					std::shared_ptr<VertexState> surface_copy = std::make_shared<VertexState>();

					if (surface != nullptr) {
						surface->csgObjectIndex(csgobj.leaf->index);

						primitives->emplace_back(std::move(createVBOPrimitive(surface,
										   OpenCSG::Intersection,
										   csgobj.leaf->geom->getConvexity(), vertex_vbo)));

						(*surface_copy) = *(surface);

						cull = std::make_shared<VertexState>();
						cull->glBegin().emplace_back([]() { PRINTD("glCullFace(GL_BACK)"); glCullFace(GL_BACK); });
						vertex_states->emplace_back(std::move(cull));
						
						vertex_states->emplace_back(std::move(surface_copy));

						cull = std::make_shared<VertexState>();
						cull->glEnd().emplace_back([]() { PRINTD("glDisable(GL_CULL_FACE)"); glDisable(GL_CULL_FACE); });
						vertex_states->emplace_back(std::move(cull));
					} else {
						PRINTD("Failed to set intersection surface");
					}
				}
			}
		}

		for(const auto &csgobj : product.subtractions) {
			if (csgobj.leaf->geom) {
				vertex_array.writeIndex(0);
				PRINTD("OpenCSG product subtractions");
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

				PRINTDB("colormode = %s",
					((colormode == ColorMode::NONE) ? "ColorMode::NONE" :
					(colormode == ColorMode::HIGHLIGHT) ? "ColorMode::HIGHLIGHT" :
					(colormode == ColorMode::BACKGROUND) ? "ColorMode::BACKGROUND" :
					(colormode == ColorMode::CUTOUT) ? "ColorMode::CUTOUT" :
					(colormode == ColorMode::MATERIAL) ? "ColorMode::MATERIAL" : "UNKNOWN")
				);

				Color4f color; // = setColor(colormode, c.data(), shaderinfo);
				if (getShaderColor(colormode, c, color)) {
					last_color = color;
				}

				// negative objects should only render rear faces
				std::shared_ptr<VertexState> cull = std::make_shared<VertexState>();
				cull->glBegin().emplace_back([]() { PRINTD("glEnable(GL_CULL_FACE)"); glEnable(GL_CULL_FACE); });
				cull->glBegin().emplace_back([]() { PRINTD("glCullFace(GL_FRONT)"); glCullFace(GL_FRONT); });
				vertex_states->emplace_back(std::move(cull));

				this->create_surface(csgobj.leaf->geom, vertex_array, csgmode, csgobj.leaf->matrix, last_color);

				std::shared_ptr<OpenCSGVertexState> surface = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex_states->back());
				if (surface != nullptr) {
					surface->csgObjectIndex(csgobj.leaf->index);

					primitives->emplace_back(std::move(createVBOPrimitive(surface,
									   OpenCSG::Subtraction,
									   csgobj.leaf->geom->getConvexity(), vertex_vbo)));
				} else {
					PRINTD("Failed to set difference surface");
				}
				
				cull = std::make_shared<VertexState>();
				cull->glEnd().emplace_back([]() { PRINTD("glDisable(GL_CULL_FACE)"); glDisable(GL_CULL_FACE); });
				vertex_states->emplace_back(std::move(cull));
			}
		}
		
		vertex_array.createInterleavedVBO(vertex_vbo);
		vbo_vertex_products.emplace_back(std::make_unique<OpenCSGVBOProduct>(std::move(primitives), std::move(vertex_states), vertex_vbo));
	}
#endif // ENABLE_OPENCSG
}

void OpenCSGRenderer::renderCSGProducts(const shared_ptr<CSGProducts> &products, bool showedges, 
					const Renderer::shaderinfo_t *shaderinfo,
					bool highlight_mode, bool background_mode) const
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
			if (shaderinfo && shaderinfo->progid) {
				if (shaderinfo->type != EDGE_RENDERING ||
					(shaderinfo->type == EDGE_RENDERING && showedges))
					glUseProgram(shaderinfo->progid);
			}
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

				const Color4f color = setColor(colormode, c.data(), shaderinfo);
				if (color[3] == 1.0f) {
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

				const Color4f color = setColor(colormode, c.data(), shaderinfo);
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
		for (const auto &product : this->vbo_vertex_products) {
			if (product->primitives().size() > 1) {
				OpenCSG::render(product->primitives());
				glDepthFunc(GL_EQUAL);
			}

			if (shaderinfo && shaderinfo->progid) {
				PRINTD("OpenCSGVBO using shader");
				glUseProgram(shaderinfo->progid);
			}

			glBindBuffer(GL_ARRAY_BUFFER, product->vbo());

			for(const auto &vertex : product->states()) {
				if (vertex) {
					std::shared_ptr<OpenCSGVertexState> csg_vertex = std::dynamic_pointer_cast<OpenCSGVertexState>(vertex);
					if (csg_vertex != nullptr) {
						if (shaderinfo && shaderinfo->type == Renderer::SELECT_RENDERING) {
							glUniform3f(shaderinfo->data.select_rendering.identifier,
									((csg_vertex->csgObjectIndex() >> 0) & 0xff) / 255.0f,
									((csg_vertex->csgObjectIndex() >> 8) & 0xff) / 255.0f,
									((csg_vertex->csgObjectIndex() >> 16) & 0xff) / 255.0f);
						}
					}
					vertex->drawArrays();
				}
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			if (shaderinfo) glUseProgram(0);
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
