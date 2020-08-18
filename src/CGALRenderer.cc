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

#ifdef _MSC_VER 
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

// dxfdata.h must come first for Eigen SIMD alignment issues
#include "dxfdata.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "printutils.h"
#include "feature.h"

#include "CGALRenderer.h"

//#include "Preferences.h"

CGALRenderer::CGALRenderer(shared_ptr<const class Geometry> geom)
	: last_render_state(Feature::ExperimentalVxORenderers.is_enabled()) // FIXME: this is temporary to make switching between renderers seamless.
{
	this->addGeometry(geom);
}

void CGALRenderer::addGeometry(const shared_ptr<const Geometry> &geom)
{
	if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		for (const auto &item : geomlist->getChildren()) {
			this->addGeometry(item.second);
		}
	}
	else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
		assert(ps->getDimension() == 3);
		// We need to tessellate here, in case the generated PolySet contains concave polygons
    // See testdata/scad/3D/features/polyhedron-concave-test.scad
		auto ps_tri = new PolySet(3, ps->convexValue());
		ps_tri->setConvexity(ps->getConvexity());
		PolysetUtils::tessellate_faces(*ps, *ps_tri);
		this->polysets.push_back(shared_ptr<const PolySet>(ps_tri));
	}
	else if (const auto poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
		this->polysets.push_back(shared_ptr<const PolySet>(poly->tessellate()));
	}
	else if (const auto new_N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		assert(new_N->getDimension() == 3);
		if (!new_N->isEmpty()) {
			this->nefPolyhedrons.push_back(new_N);
		}
	}
}

CGALRenderer::~CGALRenderer()
{
}

const std::list<shared_ptr<class CGAL_OGL_Polyhedron> > &CGALRenderer::getPolyhedrons() const
{
	if (!this->nefPolyhedrons.empty() &&
	    (this->polyhedrons.empty() || Feature::ExperimentalVxORenderers.is_enabled() != last_render_state)) // FIXME: this is temporary to make switching between renderers seamless.
	    buildPolyhedrons();
	return this->polyhedrons;
}

void CGALRenderer::buildPolyhedrons() const
{
	PRINTD("buildPolyhedrons");
	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		this->polyhedrons.clear();

		for(const auto &N : this->nefPolyhedrons) {
			auto p = new CGAL_OGL_Polyhedron(*this->colorscheme);
			CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*N->p3, p);
			// CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
			// CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
			p->init();
			this->polyhedrons.push_back(shared_ptr<CGAL_OGL_Polyhedron>(p));
		}
	} else {
		this->polyhedrons.clear();

		for(const auto &N : this->nefPolyhedrons) {
			auto p = new CGAL_OGL_VBOPolyhedron(*this->colorscheme);
			CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*N->p3, p);
			// CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
			// CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
			p->init();
			this->polyhedrons.push_back(shared_ptr<CGAL_OGL_Polyhedron>(p));
		}
	}
	PRINTD("buildPolyhedrons() end");
}

// Overridden from Renderer
void CGALRenderer::setColorScheme(const ColorScheme &cs)
{
	PRINTD("setColorScheme");
	Renderer::setColorScheme(cs);
	colormap[ColorMode::CGAL_FACE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_FACE_2D_COLOR);
	colormap[ColorMode::CGAL_EDGE_2D_COLOR] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_2D_COLOR);
	this->polyhedrons.clear(); // Mark as dirty
	PRINTD("setColorScheme done");
}

void CGALRenderer::createPolysets() const
{
	PRINTD("createPolysets() polyset");
	
	product_vertex_sets.clear();

	std::unique_ptr<std::vector<PCVertex>> poly_edge_buffer(new std::vector<PCVertex>());
	std::unique_ptr<std::vector<Vertex>> surface_buffer(new std::vector<Vertex>());
	std::unique_ptr<VertexSets> vertex_sets(new VertexSets());
	
	VertexSet *poly_edge_prev = nullptr;
	VertexSet *surface_prev = nullptr;
	
	GLuint poly_edge_vbo;
	GLuint surface_vbo;
	glGenBuffers(1, &poly_edge_vbo);
	glGenBuffers(1, &surface_vbo);
	
	unsigned int last_dimension = 0;

	for (const auto &polyset : this->polysets) {
		Color4f color;
		PRINTD("polysets");
		if (polyset->getDimension() == 2) {
			PRINTDB("2d polysets %d", last_dimension);
			GLintptr poly_edge_start_offset = 0;
			GLsizei poly_edge_draw_size = 0;

			if (last_dimension && last_dimension != 2) {
				// switch vertex sets
				product_vertex_sets.emplace_back(surface_vbo,std::move(vertex_sets));
				vertex_sets = std::unique_ptr<VertexSets>(new VertexSets());
			}

			if (poly_edge_prev) {
				poly_edge_start_offset = poly_edge_prev->start_offset;
				poly_edge_draw_size = poly_edge_prev->draw_size;
			}

			// Create 2D polygons
			getColor(ColorMode::CGAL_FACE_2D_COLOR, color);
			this->create_polygons(polyset, *poly_edge_buffer, *(vertex_sets.get()),
						VertexSet({PC, false, OpenSCADOperator::UNION, 0, 0, 0, 0, false, false, true, false, false, 0}),
						poly_edge_start_offset, poly_edge_draw_size,
						CSGMODE_NONE, Transform3d::Identity(), color);

			poly_edge_prev = (vertex_sets->empty() ? nullptr : vertex_sets->back().get());
			if (poly_edge_prev) {
				poly_edge_start_offset = poly_edge_prev->start_offset;
				poly_edge_draw_size = poly_edge_prev->draw_size;
			}

			PRINTDB("vertex_sets->size = %d", vertex_sets->size());
			// Create 2D edges
			getColor(ColorMode::CGAL_EDGE_2D_COLOR, color);
			this->create_edges(polyset, *poly_edge_buffer, *vertex_sets,
					   VertexSet({PC, false, OpenSCADOperator::UNION, 0, 0, 0, 0, false, false, false, true, true, 0}),
					   poly_edge_start_offset, poly_edge_draw_size, CSGMODE_NONE, Transform3d::Identity(), color);

			poly_edge_prev = (vertex_sets->empty() ? nullptr : vertex_sets->back().get());
		} else {
			VertexSet *vertex_set = nullptr;
			PRINTDB("3d polysets %d", last_dimension);

			if (last_dimension && last_dimension == 2) {
				// switch vertex sets
				product_vertex_sets.emplace_back(poly_edge_vbo,std::move(vertex_sets));
				vertex_sets = std::unique_ptr<VertexSets>(new VertexSets());
			}

			GLintptr surface_start_offset = 0;
			GLsizei surface_draw_size = 0;
			if (surface_prev) {
				surface_start_offset = surface_prev->start_offset;
				surface_draw_size = surface_prev->draw_size;
			}

			// Create 3D polygons
			vertex_set = new VertexSet({SHADER, false, OpenSCADOperator::UNION, 0, 0, 0, 0, false, false, false, false, false, 0});

			getColor(ColorMode::MATERIAL, color);
			this->create_surface(polyset, *surface_buffer, *vertex_set,
						surface_start_offset, surface_draw_size, CSGMODE_NORMAL,
						Transform3d::Identity(), color);


			vertex_sets->emplace_back(vertex_set);
			surface_prev = (vertex_sets->empty() ? nullptr : vertex_sets->back().get());
		}
	
		last_dimension = polyset->getDimension();
	}
	
	if (this->polysets.size()) {
		if (last_dimension == 2) {
			product_vertex_sets.emplace_back(poly_edge_vbo,std::move(vertex_sets));
		} else {
			product_vertex_sets.emplace_back(surface_vbo,std::move(vertex_sets));		
		}
	}

	if (poly_edge_buffer->size()) {
		glBindBuffer(GL_ARRAY_BUFFER, poly_edge_vbo);
		glBufferData(GL_ARRAY_BUFFER, poly_edge_buffer->size()*sizeof(Vertex), poly_edge_buffer->data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		poly_edge_buffer->clear();
	} else {
		glDeleteBuffers(1,&poly_edge_vbo);
	}
	
	if (surface_buffer->size()) {
		glBindBuffer(GL_ARRAY_BUFFER, surface_vbo);
		glBufferData(GL_ARRAY_BUFFER, surface_buffer->size()*sizeof(Vertex), surface_buffer->data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		surface_buffer->clear();
	} else {
		glDeleteBuffers(1,&surface_vbo);
	}
}

void CGALRenderer::draw(bool showfaces, bool showedges) const
{
	PRINTD("draw()");
	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		for (const auto &polyset : this->polysets) {
			PRINTD("draw() polyset");
			if (polyset->getDimension() == 2) {
				// Draw 2D polygons
				glDisable(GL_LIGHTING);
				setColor(ColorMode::CGAL_FACE_2D_COLOR);
				
				for (const auto &polygon : polyset->polygons) {
					glBegin(GL_POLYGON);
					for (const auto &p : polygon) {
						glVertex3d(p[0], p[1], 0);
					}
					glEnd();
				}

				// Draw 2D edges
				glDisable(GL_DEPTH_TEST);

				glLineWidth(2);
				setColor(ColorMode::CGAL_EDGE_2D_COLOR);
				this->render_edges(polyset, CSGMODE_NONE);
				glEnable(GL_DEPTH_TEST);
			}
			else {
				// Draw 3D polygons
				setColor(ColorMode::MATERIAL);
				this->render_surface(polyset, CSGMODE_NORMAL, Transform3d::Identity(), nullptr);
			}
		}
	} else {
		PRINTDB("product_vertex_sets.size = %d", product_vertex_sets.size());
		if (!product_vertex_sets.size()) createPolysets();
		
		// grab current state to restore after
		GLfloat current_point_size, current_line_width;
		GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
		GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
		GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

		glGetFloatv(GL_LINE_WIDTH, &current_line_width);

		for (const auto &product : product_vertex_sets) {
			glBindBuffer(GL_ARRAY_BUFFER, product.first);
			PRINTDB("drawing vertex_sets->size = %d", product.second->size());

			for (const auto &vertex_set : *product.second) {

				if (vertex_set->change_lighting)
					glDisable(GL_LIGHTING);
				if (vertex_set->change_depth_test)
					glDisable(GL_DEPTH_TEST);
				if (vertex_set->change_linewidth)
					glLineWidth(2);

				if (vertex_set->vertex_type == SHADER) {
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_NORMAL_ARRAY);
					glEnableClientState(GL_COLOR_ARRAY);
					glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (GLvoid *)(vertex_set->start_offset));
					glNormalPointer(GL_FLOAT, sizeof(Vertex), (GLvoid *)(vertex_set->start_offset + (sizeof(float)*3)));
					glColorPointer(4, GL_FLOAT, sizeof(Vertex), (GLvoid *)((vertex_set->start_offset + (sizeof(float)*3)*2)));
				} else if (vertex_set->vertex_type == PNC) {
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_NORMAL_ARRAY);
					glEnableClientState(GL_COLOR_ARRAY);
					glVertexPointer(3, GL_FLOAT, sizeof(PNCVertex), (GLvoid *)(vertex_set->start_offset));
					glNormalPointer(GL_FLOAT, sizeof(PNCVertex), (GLvoid *)(vertex_set->start_offset + (sizeof(float)*3)));
					glColorPointer(4, GL_FLOAT, sizeof(PNCVertex), (GLvoid *)((vertex_set->start_offset + (sizeof(float)*3)*2)));
				} else {
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_COLOR_ARRAY);
					glVertexPointer(3, GL_FLOAT, sizeof(PCVertex), (GLvoid *)(vertex_set->start_offset));
					glColorPointer(4, GL_FLOAT, sizeof(PCVertex), (GLvoid *)(vertex_set->start_offset + (sizeof(float)*3)));
				}

				PRINTDB("draw draw_type = %d, draw_size = %d", vertex_set->draw_type % vertex_set->draw_size);
				glDrawArrays(vertex_set->draw_type, 0, vertex_set->draw_size);

				if (vertex_set->change_depth_test)
					glEnable(GL_DEPTH_TEST);
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// restore states
		glPointSize(current_point_size);
		glLineWidth(current_line_width);

		if (!origVertexArrayState) glDisableClientState(GL_VERTEX_ARRAY);
		if (!origNormalArrayState) glDisableClientState(GL_NORMAL_ARRAY);
		if (!origColorArrayState) glDisableClientState(GL_COLOR_ARRAY);
	}

	for (const auto &p : this->getPolyhedrons()) {
		*const_cast<bool *>(&last_render_state) = Feature::ExperimentalVxORenderers.is_enabled(); // FIXME: this is temporary to make switching between renderers seamless.
		if (showfaces) p->set_style(SNC_BOUNDARY);
		else p->set_style(SNC_SKELETON);
		p->draw(showfaces && showedges);
	}

	PRINTD("draw() end");
}

BoundingBox CGALRenderer::getBoundingBox() const
{
	BoundingBox bbox;

  	for (const auto &p : this->getPolyhedrons()) {
		CGAL::Bbox_3 cgalbbox = p->bbox();
		bbox.extend(BoundingBox(
			Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
			Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax())));
	}
	for (const auto &ps : this->polysets) {
		bbox.extend(ps->getBoundingBox());
	}
	return bbox;
}
