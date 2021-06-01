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
	: last_render_state(Feature::ExperimentalVxORenderers.is_enabled()), // FIXME: this is temporary to make switching between renderers seamless.
	  polyset_vertices_vbo(0), polyset_elements_vbo(0)
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
	
	if (!this->nefPolyhedrons.empty() && this->polyhedrons.empty())
		createPolyhedrons();
}

CGALRenderer::~CGALRenderer()
{
	if (polyset_vertices_vbo) {
		glDeleteBuffers(1, &polyset_vertices_vbo);
	}
	if (polyset_elements_vbo) {
		glDeleteBuffers(1, &polyset_elements_vbo);
	}
}

void CGALRenderer::createPolyhedrons()
{
	PRINTD("createPolyhedrons");
	this->polyhedrons.clear();

	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		for(const auto &N : this->nefPolyhedrons) {
			auto p = new CGAL_OGL_Polyhedron(*this->colorscheme);
			CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*N->p3, p);
			// CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
			// CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
			p->init();
			this->polyhedrons.push_back(shared_ptr<CGAL_OGL_Polyhedron>(p));
		}
	} else {
		for(const auto &N : this->nefPolyhedrons) {
			auto p = new CGAL_OGL_VBOPolyhedron(*this->colorscheme);
			CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*N->p3, p);
			// CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
			// CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
			p->init();
			this->polyhedrons.push_back(shared_ptr<CGAL_OGL_Polyhedron>(p));
		}
	}
	PRINTD("createPolyhedrons() end");
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

void CGALRenderer::createPolysets()
{
	PRINTD("createPolysets() polyset");
	
	polyset_states.clear();

	VertexArray vertex_array(std::make_shared<VertexStateFactory>(), polyset_states);
	vertex_array.addEdgeData();
	vertex_array.addSurfaceData();
	
	if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
		size_t vertices_size = 0, elements_size = 0;
		if (this->polysets.size()) {
			for (const auto &polyset : this->polysets) {
				vertices_size += getSurfaceBufferSize(*polyset);
				vertices_size += getEdgeBufferSize(*polyset);
			}
		}
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
			GL_TRACE("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %d, %p, GL_STATIC_DRAW)", elements_size % (void *)nullptr);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_size, nullptr, GL_STATIC_DRAW); GL_ERROR_CHECK();
		}
	} else if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
		vertex_array.addElementsData(std::make_shared<AttributeData<GLuint,1,GL_UNSIGNED_INT>>());
	}
	
	for (const auto &polyset : this->polysets) {
		Color4f color;

		PRINTD("polysets");
		if (polyset->getDimension() == 2) {
			PRINTD("2d polysets");
			vertex_array.writeEdge();

			std::shared_ptr<VertexState> init_state = std::make_shared<VertexState>();
			init_state->glEnd().emplace_back([]() {
				GL_TRACE0("glDisable(GL_LIGHTING)");
				glDisable(GL_LIGHTING); GL_ERROR_CHECK();
			});
			polyset_states.emplace_back(std::move(init_state));

			// Create 2D polygons
			getColor(ColorMode::CGAL_FACE_2D_COLOR, color);
			this->create_polygons(*polyset, vertex_array, CSGMODE_NONE, Transform3d::Identity(), color);

			std::shared_ptr<VertexState> edge_state = std::make_shared<VertexState>();
			edge_state->glBegin().emplace_back([]() {
				GL_TRACE0("glDisable(GL_DEPTH_TEST)");
				glDisable(GL_DEPTH_TEST); GL_ERROR_CHECK();
			});
			edge_state->glBegin().emplace_back([]() {
				GL_TRACE0("glLineWidth(2)");
				glLineWidth(2); GL_ERROR_CHECK();
			});
			polyset_states.emplace_back(std::move(edge_state));
			
			// Create 2D edges
			getColor(ColorMode::CGAL_EDGE_2D_COLOR, color);
			this->create_edges(*polyset, vertex_array, CSGMODE_NONE, Transform3d::Identity(), color);
			
			std::shared_ptr<VertexState> end_state = std::make_shared<VertexState>();
			end_state->glBegin().emplace_back([]() {
				GL_TRACE0("glEnable(GL_DEPTH_TEST)");
				glEnable(GL_DEPTH_TEST); GL_ERROR_CHECK();
			});
			polyset_states.emplace_back(std::move(end_state));
		} else {
			PRINTD("3d polysets");
			vertex_array.writeSurface();

			// Create 3D polygons
			getColor(ColorMode::MATERIAL, color);
			this->create_surface(*polyset, vertex_array, CSGMODE_NORMAL, Transform3d::Identity(), color);
		}
	}
	
	if (this->polysets.size()) {
		if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
				GL_TRACE0("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
			}
			GL_TRACE0("glBindBuffer(GL_ARRAY_BUFFER, 0)");
			glBindBuffer(GL_ARRAY_BUFFER, 0); GL_ERROR_CHECK();
		}

		vertex_array.createInterleavedVBOs();
		polyset_vertices_vbo = vertex_array.verticesVBO();
		polyset_elements_vbo = vertex_array.elementsVBO();
	}
}

void CGALRenderer::prepare(bool showfaces, bool showedges, const shaderinfo_t * /*shaderinfo*/)
{
	PRINTD("prepare()");
	if (!polyset_states.size()) createPolysets();
	if (!this->nefPolyhedrons.empty() &&
	    (this->polyhedrons.empty() || Feature::ExperimentalVxORenderers.is_enabled() != last_render_state)) // FIXME: this is temporary to make switching between renderers seamless.
		createPolyhedrons();

	PRINTD("prepare() end");
}

void CGALRenderer::draw(bool showfaces, bool showedges, const shaderinfo_t * /*shaderinfo*/) const
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
				this->render_edges(*polyset, CSGMODE_NONE);
				glEnable(GL_DEPTH_TEST);
			}
			else {
				// Draw 3D polygons
				setColor(ColorMode::MATERIAL);
				this->render_surface(*polyset, CSGMODE_NORMAL, Transform3d::Identity(), nullptr);
			}
		}
	} else {
		// grab current state to restore after
		GLfloat current_point_size, current_line_width;
		GLboolean origVertexArrayState = glIsEnabled(GL_VERTEX_ARRAY);
		GLboolean origNormalArrayState = glIsEnabled(GL_NORMAL_ARRAY);
		GLboolean origColorArrayState = glIsEnabled(GL_COLOR_ARRAY);

		glGetFloatv(GL_POINT_SIZE, &current_point_size); GL_ERROR_CHECK();
		glGetFloatv(GL_LINE_WIDTH, &current_line_width); GL_ERROR_CHECK();

		for (const auto &polyset : polyset_states) {
			if (polyset) polyset->draw();
		}

		// restore states
		GL_TRACE("glPointSize(%d)", current_point_size);
		glPointSize(current_point_size); GL_ERROR_CHECK();
		GL_TRACE("glLineWidth(%d)", current_line_width);
		glLineWidth(current_line_width); GL_ERROR_CHECK();

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
