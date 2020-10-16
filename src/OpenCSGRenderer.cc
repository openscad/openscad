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

#endif

OpenCSGRenderer::OpenCSGRenderer(shared_ptr<CSGProducts> root_products,
																 shared_ptr<CSGProducts> highlights_products,
																 shared_ptr<CSGProducts> background_products,
																 GLView::shaderinfo_t *shaderinfo)
	: root_products(root_products),
		highlights_products(highlights_products),
		background_products(background_products), shaderinfo(shaderinfo)
{
}

void OpenCSGRenderer::draw(bool /*showfaces*/, bool showedges) const
{
	GLView::shaderinfo_t *shaderinfo = this->shaderinfo;
	if (!shaderinfo->progid) shaderinfo = nullptr;
	if (!showedges) shaderinfo = nullptr;

	this->draw_with_shader(shaderinfo);
}

void OpenCSGRenderer::draw_with_shader(const GLView::shaderinfo_t *shaderinfo) const
{
	if (this->root_products) {
		renderCSGProducts(*this->root_products, shaderinfo, false, false);
	}
	if (this->background_products) {
		renderCSGProducts(*this->background_products, shaderinfo, false, true);
	}
	if (this->highlights_products) {
		renderCSGProducts(*this->highlights_products, shaderinfo, true, false);
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

void OpenCSGRenderer::renderCSGProducts(const CSGProducts &products, const GLView::shaderinfo_t *shaderinfo,
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
		OPENGL_TEST("start");
		if (shaderinfo) glUseProgram(shaderinfo->progid);
		OPENGL_TEST("load shader");

		for (const auto &csgobj : product.intersections) {
			if (shaderinfo && shaderinfo->type == GLView::shaderinfo_t::SELECT_RENDERING) {
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
#endif
}

BoundingBox OpenCSGRenderer::getBoundingBox() const
{
	BoundingBox bbox;
	if (this->root_products) bbox = this->root_products->getBoundingBox();
	if (this->highlights_products) bbox.extend(this->highlights_products->getBoundingBox());
	if (this->background_products) bbox.extend(this->background_products->getBoundingBox());


	return bbox;
}
