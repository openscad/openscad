// Copyright (c) 1997-2002  Max-Planck-Institute Saarbruecken (Germany).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL: svn+ssh://scm.gforge.inria.fr/svn/cgal/trunk/Nef_3/include/CGAL/Nef_3/OGL_helper.h $
// $Id: OGL_helper.h 56667 2010-06-09 07:37:13Z sloriot $
// 
//
// Author(s)     : Peter Hachenberger <hachenberger@mpi-sb.mpg.de>

// Modified for OpenSCAD

#pragma once

#include "system-gl.h"
#include "VBORenderer.h"
#include "OGL_helper.h"
#include <cstdlib>

namespace CGAL {

namespace OGL {

// ----------------------------------------------------------------------------
// OGL Drawable Polyhedron:
// ----------------------------------------------------------------------------
class VBOPolyhedron : public virtual Polyhedron {
public:
	VBOPolyhedron()
		: Polyhedron(),
		  points_edges_vertices_vbo(0), points_edges_elements_vbo(0),
		  halffacets_vertices_vbo(0), halffacets_elements_vbo(0)
	{}
	virtual ~VBOPolyhedron()
	{
		if (points_edges_vertices_vbo) glDeleteBuffers(1, &points_edges_vertices_vbo);
		if (points_edges_elements_vbo) glDeleteBuffers(1, &points_edges_elements_vbo);
		if (halffacets_vertices_vbo) glDeleteBuffers(1, &halffacets_vertices_vbo);
		if (halffacets_elements_vbo) glDeleteBuffers(1, &halffacets_elements_vbo);
	}

	void draw(Vertex_iterator v, VertexArray &vertex_array) const { 
		PRINTD("draw(Vertex_iterator)");
		
		CGAL::Color c = getVertexColor(v);
		vertex_array.createVertex({Vector3d((float)v->x(), (float)v->y(), (float)v->z())},
						{},
						{(float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0},
						0, 0, 0.0, 1, 1);
	}

	void draw(Edge_iterator e, VertexArray &vertex_array) const { 
		PRINTD("draw(Edge_iterator)");
		
		Double_point p = e->source(), q = e->target();
		CGAL::Color c = getEdgeColor(e);
		Color4f color = {(float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0};

		vertex_array.createVertex({Vector3d((float)p.x(), (float)p.y(), (float)p.z())},
						{},
						color,
						0, 0, 0.0, 1, 2, true);
		vertex_array.createVertex({Vector3d((float)q.x(), (float)q.y(), (float)q.z())},
						{},
						color,
						0, 1, 0.0, 1, 2, true);
	}

	typedef struct _TessUserData {
		GLenum which;
		GLdouble *normal;
		CGAL::Color color;
		size_t primitive_index;
		size_t active_point_index;
		size_t last_size;
		size_t draw_size;
		size_t elements_offset;
		VertexArray &vertex_array;
	} TessUserData;

	static inline void CGAL_GLU_TESS_CALLBACK beginCallback(GLenum which, GLvoid *user) {
		TessUserData *tess(static_cast<TessUserData *>(user));
		// Create separate vertex set since "which" could be different draw type
		tess->which = which;
		tess->draw_size = 0;
		
		tess->last_size = tess->vertex_array.data()->sizeInBytes();
		tess->elements_offset = 0;
		if (tess->vertex_array.useElements()) {
			tess->elements_offset = tess->vertex_array.elements().sizeInBytes();
			// this can vary size if polyset provides triangles
			tess->vertex_array.addElementsData(std::make_shared<AttributeData<GLuint,1,GL_UNSIGNED_INT>>());
			tess->vertex_array.elementsMap().clear();
		}
	}

	static inline void CGAL_GLU_TESS_CALLBACK endCallback(GLvoid *user) {
		TessUserData *tess(static_cast<TessUserData *>(user));

		GLenum elements_type = 0;
		if (tess->vertex_array.useElements())
			elements_type = tess->vertex_array.elementsData()->glType();
		std::shared_ptr<VertexState> vs = tess->vertex_array.createVertexState(
			tess->which, tess->draw_size, elements_type,
			tess->vertex_array.writeIndex(), tess->elements_offset);
		tess->vertex_array.states().emplace_back(std::move(vs));
		tess->vertex_array.addAttributePointers(tess->last_size);
		tess->primitive_index++;
		PRINTDB("primitive_count = %d, vertices = %d", tess->primitive_index % tess->active_point_index);
	}

	static inline void CGAL_GLU_TESS_CALLBACK errorCallback(GLenum errorCode) {
		const GLubyte *estring;
		estring = gluErrorString(errorCode);
		fprintf(stderr, "Tessellation Error: %s\n", estring);
		std::exit (0);
	}
	
	static inline void CGAL_GLU_TESS_CALLBACK vertexCallback(GLvoid* vertex, GLvoid* user) {
		GLdouble* pc(static_cast<GLdouble*>(vertex));
		TessUserData *tess(static_cast<TessUserData *>(user));
		size_t shape_size = 0;
		
		switch (tess->which) {
			case GL_TRIANGLES:
			case GL_TRIANGLE_FAN:
			case GL_TRIANGLE_STRIP:
				shape_size = 3;
				break;
			case GL_POINTS:
				shape_size = 1;
				break;
			default:
				break;
		}
		
		
		tess->vertex_array.createVertex({Vector3d((float)pc[0], (float)pc[1], (float)pc[2])},
						{Vector3d((float)(tess->normal[0]), (float)(tess->normal[1]), (float)(tess->normal[2]))},
						{(float)(tess->color.red()/255.0f), (float)(tess->color.green()/255.0f), (float)(tess->color.blue()/255.0f), 1.0},
						0, 0, 0.0, shape_size, 3);
		tess->draw_size++;
		tess->active_point_index++;
	}

	static inline void CGAL_GLU_TESS_CALLBACK combineCallback(GLdouble coords[3], GLvoid *[4], GLfloat [4], GLvoid **dataOut) {
		static std::list<GLdouble*> pcache;
		if (dataOut) {
			GLdouble *n = new GLdouble[3];
			n[0] = coords[0];
			n[1] = coords[1];
			n[2] = coords[2];
			pcache.push_back(n);
			*dataOut = n;
		} else {
			for (std::list<GLdouble*>::const_iterator i = pcache.begin(); i != pcache.end(); i++)
			delete[] *i;
			pcache.clear();
		}
	}

	void draw(Halffacet_iterator f, VertexArray &vertex_array, bool is_back_facing) const {
		PRINTD("draw(Halffacet_iterator)");
		
		GLUtesselator* tess_ = gluNewTess();
		gluTessCallback(tess_, GLenum(GLU_TESS_VERTEX_DATA),
			      (GLvoid (CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) &vertexCallback);
		gluTessCallback(tess_, GLenum(GLU_TESS_COMBINE),
			      (GLvoid (CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) &combineCallback);
		gluTessCallback(tess_, GLenum(GLU_TESS_BEGIN_DATA),
			      (GLvoid (CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) &beginCallback);
		gluTessCallback(tess_, GLenum(GLU_TESS_END_DATA),
			      (GLvoid (CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) &endCallback);
		gluTessCallback(tess_, GLenum(GLU_TESS_ERROR),
			      (GLvoid (CGAL_GLU_TESS_CALLBACK *)(CGAL_GLU_TESS_DOTS)) &errorCallback);
		gluTessProperty(tess_, GLenum(GLU_TESS_WINDING_RULE),
			      GLU_TESS_WINDING_POSITIVE);

		DFacet::Coord_const_iterator cit;
		TessUserData tess_data = {
			0, f->normal(), getFacetColor(f,is_back_facing),
			0, 0, 0, 0, 0, vertex_array
		};
		
		gluTessBeginPolygon(tess_,&tess_data);
		// forall facet cycles of f:
		for(unsigned i = 0; i < f->number_of_facet_cycles(); ++i) {
			gluTessBeginContour(tess_);
			// put all vertices in facet cycle into contour:
			for(cit = f->facet_cycle_begin(i); 
				cit != f->facet_cycle_end(i); ++cit) {
				gluTessVertex(tess_, *cit, *cit);
			}
			gluTessEndContour(tess_);
		}
		gluTessEndPolygon(tess_);
		gluDeleteTess(tess_);
		combineCallback(NULL, NULL, NULL, NULL);
	}

	void create_polyhedron() {
		PRINTD("create_polyhedron");

		VertexArray points_edges_array(std::make_shared<VertexStateFactory>(), points_edges_states);
		points_edges_array.addEdgeData();
		points_edges_array.writeEdge();
		size_t last_size = 0;
		size_t elements_offset = 0;
		
		size_t vertices_size = vertices_.size() + edges_.size()*2, elements_size = 0;
		vertices_size *= points_edges_array.stride();
		if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
			if (vertices_size <= 0xff) {
				points_edges_array.addElementsData(std::make_shared<AttributeData<GLubyte,1,GL_UNSIGNED_BYTE>>());
			} else if (vertices_size <= 0xffff) {
				points_edges_array.addElementsData(std::make_shared<AttributeData<GLushort,1,GL_UNSIGNED_SHORT>>());
			} else {
				points_edges_array.addElementsData(std::make_shared<AttributeData<GLuint,1,GL_UNSIGNED_INT>>());
			}
			elements_size = vertices_size * points_edges_array.elements().stride();
		}

		if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			points_edges_array.verticesSize(vertices_size);
			points_edges_array.elementsSize(elements_size);

			glBindBuffer(GL_ARRAY_BUFFER, points_edges_array.verticesVBO());
			glBufferData(GL_ARRAY_BUFFER, vertices_size, nullptr, GL_STATIC_DRAW);
			if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, points_edges_array.elementsVBO());
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_size, nullptr, GL_STATIC_DRAW);
			}
		}		
		
		// Points
		Vertex_iterator v;
		if (points_edges_array.useElements()) {
			elements_offset = points_edges_array.elementsOffset();
			points_edges_array.elementsMap().clear();
		}

		std::shared_ptr<VertexState> settings = std::make_shared<VertexState>();
		settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisable(GL_LIGHTING)"); glDisable(GL_LIGHTING); });
		settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glPointSize(10.0f)"); glPointSize(10.0f); });
		points_edges_states.emplace_back(std::move(settings));
		
	        for(v=vertices_.begin();v!=vertices_.end();++v) 
			draw(v, points_edges_array);
		
		GLenum elements_type = 0;
		if (points_edges_array.useElements())
			elements_type = points_edges_array.elementsData()->glType();
		std::shared_ptr<VertexState> vs = points_edges_array.createVertexState(
			GL_POINTS, vertices_.size(), elements_type,
			points_edges_array.writeIndex(), elements_offset);
		points_edges_states.emplace_back(std::move(vs));
		points_edges_array.addAttributePointers(last_size);
		
		// Edges
		Edge_iterator e;
		last_size = points_edges_array.verticesOffset();
		elements_offset = 0;
		if (points_edges_array.useElements()) {
			elements_offset = points_edges_array.elementsOffset();
			points_edges_array.elementsMap().clear();
		}

		settings = std::make_shared<VertexState>();
		settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisable(GL_LIGHTING)"); glDisable(GL_LIGHTING); });
		settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glLineWidth(5.0f)"); glLineWidth(5.0f); });
		points_edges_states.emplace_back(std::move(settings));

	        for(e=edges_.begin();e!=edges_.end();++e)
			draw(e, points_edges_array);


		elements_type = 0;
		if (points_edges_array.useElements())
			elements_type = points_edges_array.elementsData()->glType();
		vs = points_edges_array.createVertexState(
			GL_LINES, edges_.size()*2, elements_type,
			points_edges_array.writeIndex(), elements_offset);
		points_edges_states.emplace_back(std::move(vs));
		points_edges_array.addAttributePointers(last_size);

		if (Feature::ExperimentalVxORenderersDirect.is_enabled() || Feature::ExperimentalVxORenderersPrealloc.is_enabled()) {
			if (Feature::ExperimentalVxORenderersIndexing.is_enabled()) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		points_edges_array.createInterleavedVBOs();
		points_edges_vertices_vbo = points_edges_array.verticesVBO();
		points_edges_elements_vbo = points_edges_array.elementsVBO();
		
		// Halffacets
		VertexArray halffacets_array(std::make_shared<VertexStateFactory>(), halffacets_states);
		halffacets_array.addSurfaceData();
		halffacets_array.writeSurface();
		last_size = 0;

		settings = std::make_shared<VertexState>();
		settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glEnable(GL_LIGHTING)"); glEnable(GL_LIGHTING); });
		settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glLineWidth(5.0f)"); glLineWidth(5.0f); });
		if (cull_backfaces || color_backfaces) {
			settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glEnable(GL_CULL_FACE)"); glEnable(GL_CULL_FACE); });
			settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_BACK)"); glCullFace(GL_BACK); });
		}
		halffacets_states.emplace_back(std::move(settings));

		for (int i = 0; i < (color_backfaces ? 2 : 1); i++) {

			Halffacet_iterator f;
			for(f=halffacets_.begin();f!=halffacets_.end();++f)
				draw(f, halffacets_array, i);

			if (color_backfaces) {
				settings = std::make_shared<VertexState>();
				settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_FRONT)"); glCullFace(GL_FRONT); });
				halffacets_states.emplace_back(std::move(settings));
			}
		}

		if (cull_backfaces || color_backfaces) {
			settings = std::make_shared<VertexState>();
			settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_BACK)"); glCullFace(GL_BACK); });
			settings->glBegin().emplace_back([]() { if (OpenSCAD::debug != "") PRINTD("glDisable(GL_CULL_FACE)"); glDisable(GL_CULL_FACE); });
			halffacets_states.emplace_back(std::move(settings));
		}
		
		halffacets_array.createInterleavedVBOs();
		halffacets_vertices_vbo = halffacets_array.verticesVBO();
		halffacets_elements_vbo = halffacets_array.elementsVBO();
	}

	void init() override { 
        	PRINTD("VBO init()");
        	create_polyhedron();
        	PRINTD("VBO init() end");
        }
	
	void draw() const override {
		PRINTD("VBO draw()");
		PRINTD("VBO draw() end");
	}

protected:
	GLuint points_edges_vertices_vbo;
	GLuint points_edges_elements_vbo;
	GLuint halffacets_vertices_vbo;
	GLuint halffacets_elements_vbo;
	VertexStates points_edges_states;
	VertexStates halffacets_states;
}; // Polyhedron

} // namespace OGL

} //namespace CGAL
