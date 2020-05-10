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

#ifdef ENABLE_OPENCSG
#include <opencsg.h>

#ifndef ENABLE_EXPERIMENTAL
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
#else
class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity, const OpenCSGRenderer &renderer, const VBORenderer::VertexSet &vertex_set, const GLuint vbo) :
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
#endif // ENABLE_EXPERIMENTAL
#endif // ENABLE_OPENCSG

//
// Original OpenCSGRenderer code
//
#ifndef ENABLE_EXPERIMENTAL
OpenCSGRenderer::OpenCSGRenderer(shared_ptr<CSGProducts> root_products,
																 shared_ptr<CSGProducts> highlights_products,
																 shared_ptr<CSGProducts> background_products,
																 GLint *shaderinfo)
	: root_products(root_products), 
		highlights_products(highlights_products), 
		background_products(background_products), shaderinfo(shaderinfo)
{
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	GLint *shaderinfo = this->shaderinfo;
	if (!shaderinfo[0]) shaderinfo = nullptr;
	if (this->root_products) {
		renderCSGProducts(*this->root_products, showedges ? shaderinfo : nullptr, false, false);
	}
	if (this->background_products) {
		renderCSGProducts(*this->background_products, showedges ? shaderinfo : nullptr, false, true);
	}
	if (this->highlights_products) {
		renderCSGProducts(*this->highlights_products, showedges ? shaderinfo : nullptr, true, false);
	}
}

OpenCSGRenderer::~OpenCSGRenderer()
{
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

void OpenCSGRenderer::renderCSGProducts(const CSGProducts &products, GLint *shaderinfo, 
										bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
	for(const auto &product : products.products) {
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
		if (shaderinfo) glUseProgram(shaderinfo[0]);

		for(const auto &csgobj : product.intersections) {
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
#endif // ENABLE_OPENCSG
}
#endif // ENABLE_EXPERIMENTAL



//
// VBORenderer OpenCSGRenderer Code
//
#ifdef ENABLE_EXPERIMENTAL
OpenCSGRenderer::OpenCSGRenderer(shared_ptr<CSGProducts> root_products,
																 shared_ptr<CSGProducts> highlights_products,
																 shared_ptr<CSGProducts> background_products,
																 GLint *shaderinfo)
	: VBORenderer(),
		product_vertex_sets(new std::vector<VertexSets *>()),
		root_products(root_products),
		highlights_products(highlights_products),
		background_products(background_products), shaderinfo(shaderinfo)
{
}

OpenCSGRenderer::~OpenCSGRenderer()
{
  if (product_vertex_sets) {
	for (auto &vertex_sets : *product_vertex_sets) {
			glDeleteBuffers(1,&vertex_sets->first);
			for (auto &vertex_set : *vertex_sets->second) {
					delete vertex_set;
			}
			vertex_sets->second->clear();
			delete vertex_sets;
	}
	product_vertex_sets->clear();
	delete product_vertex_sets;
}
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	if (!getProductVertexSets().size()) {
		GLint *shaderinfo = this->shaderinfo;
		if (!shaderinfo[0]) shaderinfo = nullptr;

		if (this->root_products) {
				renderCSGProducts(*this->root_products, showedges ? shaderinfo : nullptr, false, false);
		}
		if (this->background_products) {
				renderCSGProducts(*this->background_products, showedges ? shaderinfo : nullptr, false, true);
		}
		if (this->highlights_products) {
				renderCSGProducts(*this->highlights_products, showedges ? shaderinfo : nullptr, true, false);
		}
	}

	drawCSGProducts(showedges);
}

// Primitive for drawing using OpenCSG
OpenCSGPrim *OpenCSGRenderer::createCSGPrimitive(const VertexSet &vertex_set, const GLuint vbo) const
{
  if (vertex_set.operation == OpenSCADOperator::INTERSECTION)
    return new OpenCSGPrim(OpenCSG::Intersection, vertex_set.convexity, *this, vertex_set, vbo);

  if (vertex_set.operation == OpenSCADOperator::DIFFERENCE)
    return new OpenCSGPrim(OpenCSG::Subtraction, vertex_set.convexity, *this, vertex_set, vbo);

  return nullptr;
}

void OpenCSGRenderer::renderCSGProducts(const CSGProducts &products, GLint * /*shaderinfo*/,
										bool highlight_mode, bool background_mode) const
{
#ifdef ENABLE_OPENCSG
	std::vector<Vertex> *render_buffer = new std::vector<Vertex>();
	std::vector<VertexSet *> *vertex_sets = 0;
	VertexSet *prev = 0;
	VertexSet *vertex_set = 0;

	GLuint vbo;

	for(const auto &product : products.products) {
		// one vbo for each product
		if (product.intersections.size() || product.subtractions.size()) {
			vertex_sets = new std::vector<VertexSet *>();

			GLsizei combined_draw_size = 0;
			Color4f last_color;
			for(const auto &csgobj : product.intersections) {
				if (csgobj.leaf->geom && vertex_sets) {
					prev = (vertex_sets->empty() ? 0 : vertex_sets->back());
					vertex_set = new VertexSet({true, OpenSCADOperator::INTERSECTION, csgobj.leaf->geom->getConvexity(), 0, 0, false, false});
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

					const_cast<OpenCSGRenderer *>(this)->create_surface(csgobj.leaf->geom, *render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
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

					vertex_sets->push_back(vertex_set);
					combined_draw_size += vertex_set->draw_size;
				}
			}

			for(const auto &csgobj : product.subtractions) {
				if (csgobj.leaf->geom && vertex_sets) {
					prev = (vertex_sets->empty() ? 0 : vertex_sets->back());
					vertex_set = new VertexSet({true, OpenSCADOperator::DIFFERENCE, csgobj.leaf->geom->getConvexity(), 0, 0, false, false});
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
					const_cast<OpenCSGRenderer *>(this)->create_surface(csgobj.leaf->geom, *render_buffer, *vertex_set, prev_start_offset, prev_draw_size,
												 csgmode, csgobj.leaf->matrix, last_color);
					vertex_set->draw_cull_front = true;
					vertex_set->draw_cull_back = false;
					vertex_sets->push_back(vertex_set);
					combined_draw_size += vertex_set->draw_size;
				}
			}

			if (render_buffer->size()) {
				glGenBuffers(1, &vbo);

				product_vertex_sets->push_back(new VertexSets(vbo,vertex_sets));

				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, render_buffer->size()*sizeof(Vertex), render_buffer->data(), GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				render_buffer->clear();
			}
		}
	}
	delete render_buffer;
#endif // ENABLE_OPENCSG
}

void OpenCSGRenderer::drawCSGProducts(bool use_edge_shader) const
{
#ifdef ENABLE_OPENCSG
	for(const auto &vertex_sets : *product_vertex_sets) {
		std::vector<OpenCSG::Primitive*> primitives;

		if (vertex_sets) {
			for(const auto &vertex_set : *vertex_sets->second) {
				if (vertex_set->is_opencsg_vertex_set) {
          OpenCSG::Primitive *primative = nullptr;
          if ((primative = createCSGPrimitive(*vertex_set, vertex_sets->first)) != nullptr) {
					  primitives.push_back(primative);
          }
				}
			}

			if (primitives.size() > 1) {
				OpenCSG::render(primitives);
				glDepthFunc(GL_EQUAL);
			}
			if (use_edge_shader) glUseProgram(getVBOShaderSettings()[0]);

			glBindBuffer(GL_ARRAY_BUFFER, vertex_sets->first);
			for(const auto &vertex_set : *vertex_sets->second) {
				if (vertex_set->is_opencsg_vertex_set)
				{
					if (!vertex_set->draw_cull_front && !vertex_set->draw_cull_back) {
						draw_surface(*vertex_set, true, use_edge_shader);
					}
					if (vertex_set->draw_cull_front) {
						glEnable(GL_CULL_FACE); // cull face
						glCullFace(GL_FRONT);
						draw_surface(*vertex_set, true, use_edge_shader);
						glDisable(GL_CULL_FACE);
					}
					if (vertex_set->draw_cull_back) {
						glEnable(GL_CULL_FACE); // cull face
						glCullFace(GL_BACK);
						draw_surface(*vertex_set, true, use_edge_shader);
						glDisable(GL_CULL_FACE);
					}
				}
			}

			glBindBuffer(GL_ARRAY_BUFFER,0);

			if (use_edge_shader) glUseProgram(0);
			for(auto &p : primitives) delete p;
			glDepthFunc(GL_LEQUAL);
		}
	}
#endif // ENABLE_OPENCSG
}
#endif // ENABLE_EXPERIMENTAL

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_products) bbox = this->root_products->getBoundingBox();
	if (this->highlights_products) bbox.extend(this->highlights_products->getBoundingBox());
	if (this->background_products) bbox.extend(this->background_products->getBoundingBox());


	return bbox;
}
