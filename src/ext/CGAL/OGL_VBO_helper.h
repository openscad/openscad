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
		: Polyhedron(), points_edges_vbo(0), halffacets_vbo(0)
	{}
	virtual ~VBOPolyhedron()
	{
		if (points_edges_vbo != 0) glDeleteBuffers(1, &points_edges_vbo);
		if (halffacets_vbo != 0) glDeleteBuffers(1, &halffacets_vbo);
	}

	void draw(Vertex_iterator v, VertexData &vertex_data) const { 
		//      CGAL_NEF_TRACEN("drawing vertex "<<*v);
		PRINTD("draw(Vertex_iterator)");
		CGAL::Color c = getVertexColor(v);
		addAttributeValues(*vertex_data.positionData(), (float)v->x(), (float)v->y(), (float)v->z());
		if (vertex_data.hasColorData()) {
			addAttributeValues(*vertex_data.colorData(), (float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0);
		}
	}

	void draw(Edge_iterator e, VertexData &vertex_data) const { 
		//      CGAL_NEF_TRACEN("drawing edge "<<*e);
		PRINTD("draw(Edge_iterator)");
		Double_point p = e->source(), q = e->target();
		CGAL::Color c = getEdgeColor(e);
		addAttributeValues(*vertex_data.positionData(), (float)p.x(), (float)p.y(), (float)p.z());
		addAttributeValues(*vertex_data.positionData(), (float)q.x(), (float)q.y(), (float)q.z());
		if (vertex_data.hasColorData()) {
			addAttributeValues(2, *vertex_data.colorData(), (float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0);
		}
	}

	typedef struct _TessUserData {
		GLdouble *normal;
		CGAL::Color color;
		size_t last_size_in_bytes;
		size_t start_draw_size;
		VertexStates &vertex_states;
		VertexData &vertex_data;
	} TessUserData;

	static inline void CGAL_GLU_TESS_CALLBACK beginCallback(GLenum which, GLvoid *user) {
		TessUserData *tess(static_cast<TessUserData *>(user));
		// Create separate vertex set since "which" could be different draw type
		VertexData &vertex_data = tess->vertex_data;

		std::shared_ptr<VertexState> vertex_state = std::make_shared<VertexState>(which, 0);

		tess->last_size_in_bytes = vertex_data.sizeInBytes();
		tess->start_draw_size = vertex_data.size();
		tess->vertex_states.emplace_back(std::move(vertex_state));
	}

	static inline void CGAL_GLU_TESS_CALLBACK endCallback(GLvoid *user) {
		TessUserData *tess(static_cast<TessUserData *>(user));
		VertexData &vertex_data = (tess->vertex_data);
		std::shared_ptr<VertexState> vertex_state = tess->vertex_states.back();
		size_t last_size = tess->last_size_in_bytes;
		
		vertex_state->drawSize(vertex_data.size() - tess->start_draw_size);

		GLsizei count = vertex_data.positionData()->count();
		GLenum type = vertex_data.positionData()->glType();
		GLsizei stride = vertex_data.stride();
		size_t offset = last_size + vertex_data.interleavedOffset(vertex_data.positionIndex());
		vertex_state->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_VERTEX_ARRAY)");
			glEnableClientState(GL_VERTEX_ARRAY);
		});
		vertex_state->glBegin().emplace_back([count, type, stride, offset]() {
			if (OpenSCAD::debug != "") PRINTDB("glVertexPointer(%d, %d, %d, %p)", count % type % stride % offset);
			glVertexPointer(count, type, stride, (GLvoid *)offset); });
		if (vertex_data.hasNormalData()) {
			type = vertex_data.normalData()->glType();
			stride = vertex_data.stride();
			offset = last_size + vertex_data.interleavedOffset(vertex_data.normalIndex());
			vertex_state->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_NORMAL_ARRAY)");
				glEnableClientState(GL_NORMAL_ARRAY);
			});
			vertex_state->glBegin().emplace_back([type, stride, offset]() {
				if (OpenSCAD::debug != "") PRINTDB("glNormalPointer(%d, %d, %p)", type % stride % offset);
				glNormalPointer(type, stride, (GLvoid *)offset);
			});
		}
		if (vertex_data.hasColorData()) {
			count = vertex_data.colorData()->count();
			type = vertex_data.colorData()->glType();
			stride = vertex_data.stride();
			offset = last_size + vertex_data.interleavedOffset(vertex_data.colorIndex());
			vertex_state->glBegin().emplace_back([]() { 
				if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_COLOR_ARRAY)");
				glEnableClientState(GL_COLOR_ARRAY);
			});
			vertex_state->glBegin().emplace_back([count, type, stride, offset]() {
				if (OpenSCAD::debug != "") PRINTDB("glColorPointer(%d, %d, %d, %p)", count % type % stride % offset);
				glColorPointer(count, type, stride, (GLvoid *)offset);
			});
		}
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
		//GLdouble* pu(static_cast<GLdouble*>(user));
		//    CGAL_NEF_TRACEN("vertexCallback coord  "<<pc[0]<<","<<pc[1]<<","<<pc[2]);
		//    CGAL_NEF_TRACEN("vertexCallback normal "<<pu[0]<<","<<pu[1]<<","<<pu[2]);
		VertexData &halffacet_data = tess->vertex_data;
		size_t last_size = halffacet_data.sizeInBytes();

		addAttributeValues(*(halffacet_data.positionData()), (float)pc[0], (float)pc[1], (float)pc[2]);
		if (halffacet_data.hasNormalData()) {
			addAttributeValues(*(halffacet_data.normalData()), (float)(tess->normal[0]), (float)(tess->normal[1]), (float)(tess->normal[2]));
		}
		if (halffacet_data.hasColorData()) {
			addAttributeValues(*(halffacet_data.colorData()), (float)(tess->color.red()/255.0f), (float)(tess->color.green()/255.0f), (float)(tess->color.blue()/255.0f), 1.0);
		}
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

	void draw(Halffacet_iterator f, VertexStates &vertex_states, VertexData &vertex_data, bool is_back_facing) const {
		PRINTD("draw(Halffacet_iterator)");
		//      CGAL_NEF_TRACEN("drawing facet "<<(f->debug(),""));
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
			f->normal(), getFacetColor(f,is_back_facing),
			0, 0, vertex_states, vertex_data
		};
		
		gluTessBeginPolygon(tess_,&tess_data);
		//      CGAL_NEF_TRACEN(" ");
		//      CGAL_NEF_TRACEN("Begin Polygon");
		gluTessNormal(tess_,f->dx(),f->dy(),f->dz());
		// forall facet cycles of f:
		for(unsigned i = 0; i < f->number_of_facet_cycles(); ++i) {
			gluTessBeginContour(tess_);
			//	CGAL_NEF_TRACEN("  Begin Contour");
			// put all vertices in facet cycle into contour:
			for(cit = f->facet_cycle_begin(i); 
				cit != f->facet_cycle_end(i); ++cit) {
				gluTessVertex(tess_, *cit, *cit);
				//	  CGAL_NEF_TRACEN("    add Vertex");
			}
			gluTessEndContour(tess_);
			//	CGAL_NEF_TRACEN("  End Contour");
		}
		gluTessEndPolygon(tess_);
		//      CGAL_NEF_TRACEN("End Polygon");
		gluDeleteTess(tess_);
		combineCallback(NULL, NULL, NULL, NULL);
	}

	void create_polyhedron() {
		PRINTD("create_polyhedron");

		VertexData points_edges_data;
		points_edges_data.addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
		points_edges_data.addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());

		// Points
		size_t last_size = points_edges_data.sizeInBytes();
		
		Vertex_iterator v;
	        for(v=vertices_.begin();v!=vertices_.end();++v) 
			draw(v, points_edges_data);

		std::shared_ptr<VertexState> points = std::make_shared<VertexState>(GL_POINTS, vertices_.size());
		points->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glDisable(GL_LIGHTING)");
			glDisable(GL_LIGHTING);
		});
		points->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glPointSize(10.0f)");
			glPointSize(10.0f);
		});

		GLsizei count = points_edges_data.positionData()->count();
		GLenum type = points_edges_data.positionData()->glType();
		GLsizei stride = points_edges_data.stride();
		size_t offset = last_size + points_edges_data.interleavedOffset(points_edges_data.positionIndex());
		points->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_VERTEX_ARRAY)");
			glEnableClientState(GL_VERTEX_ARRAY);
		});
		points->glBegin().emplace_back([count, type, stride, offset]() {
			if (OpenSCAD::debug != "") PRINTDB("glVertexPointer(%d, %d, %d, %p)", count % type % stride % offset);
			glVertexPointer(count, type, stride, (GLvoid *)offset); });
		if (points_edges_data.hasColorData()) {
			count = points_edges_data.colorData()->count();
			type = points_edges_data.colorData()->glType();
			stride = points_edges_data.stride();
			offset = last_size + points_edges_data.interleavedOffset(points_edges_data.colorIndex());
			points->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_COLOR_ARRAY)");
				glEnableClientState(GL_COLOR_ARRAY);
			});
			points->glBegin().emplace_back([count, type, stride, offset]() {
				if (OpenSCAD::debug != "") PRINTDB("glColorPointer(%d, %d, %d, %p)", count % type % stride % offset);
				glColorPointer(count, type, stride, (GLvoid *)offset);
			});
		}
		points_edges_states.emplace_back(std::move(points));
		
		// Edges
		last_size = points_edges_data.sizeInBytes();
		Edge_iterator e;
	        for(e=edges_.begin();e!=edges_.end();++e)
			draw(e, points_edges_data);

		std::shared_ptr<VertexState> lines = std::make_shared<VertexState>(GL_LINES, edges_.size()*2);
		lines->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glDisable(GL_LIGHTING)");
			glDisable(GL_LIGHTING);
		});
		lines->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glLineWidth(5.0f)");
			glLineWidth(5.0f);
		});

		count = points_edges_data.positionData()->count();
		type = points_edges_data.positionData()->glType();
		stride = points_edges_data.stride();
		offset = last_size + points_edges_data.interleavedOffset(points_edges_data.positionIndex());
		lines->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_VERTEX_ARRAY)");
			glEnableClientState(GL_VERTEX_ARRAY);
		});
		lines->glBegin().emplace_back([count, type, stride, offset]() {
			if (OpenSCAD::debug != "") PRINTDB("glVertexPointer(%d, %d, %d, %p)", count % type % stride % offset);
			glVertexPointer(count, type, stride, (GLvoid *)offset);
		});
		if (points_edges_data.hasColorData()) {
			count = points_edges_data.colorData()->count();
			type = points_edges_data.colorData()->glType();
			stride = points_edges_data.stride();
			offset = last_size + points_edges_data.interleavedOffset(points_edges_data.colorIndex());
			lines->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glEnableClientState(GL_COLOR_ARRAY)");
				glEnableClientState(GL_COLOR_ARRAY);
			});
			lines->glBegin().emplace_back([count, type, stride, offset]() {
				if (OpenSCAD::debug != "") PRINTDB("glColorPointer(%d, %d, %d, %p)", count % type % stride % offset);
				glColorPointer(count, type, stride, (GLvoid *)offset);
			});
		}
		points_edges_states.emplace_back(std::move(lines));
		points_edges_data.createInterleavedVBO(points_edges_vbo);

		// Halffacets
		VertexData halffacets_data;
		halffacets_data.addPositionData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
		halffacets_data.addNormalData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>());
		halffacets_data.addColorData(std::make_shared<AttributeData<GLfloat,4,GL_FLOAT>>());

		std::shared_ptr<VertexState> vs = std::make_shared<VertexState>();
		vs->glBegin().emplace_back([]() {
			if (OpenSCAD::debug != "") PRINTD("glEnable(GL_LIGHTING)");
			glEnable(GL_LIGHTING);
		});
		if (cull_backfaces || color_backfaces) {
			vs->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glEnable(GL_CULL_FACE)");
				glEnable(GL_CULL_FACE);
			});
			vs->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_BACK)");
				glCullFace(GL_BACK);
			});
		}
		halffacets_states.emplace_back(std::move(vs));

	        for (int i = 0; i < (color_backfaces ? 2 : 1); i++) {
			Halffacet_iterator f;
			for(f=halffacets_.begin();f!=halffacets_.end();++f)
				draw(f, halffacets_states, halffacets_data, i);
			if (color_backfaces) {
				std::shared_ptr<VertexState> vs = std::make_shared<VertexState>();
				vs->glBegin().emplace_back([]() {
					if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_FRONT)");
					glCullFace(GL_FRONT);
				});
				halffacets_states.emplace_back(std::move(vs));
			}
		}
		if (cull_backfaces || color_backfaces) {
			std::shared_ptr<VertexState> vs = std::make_shared<VertexState>();
			vs->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glCullFace(GL_BACK)");
				glCullFace(GL_BACK);
			});
			vs->glBegin().emplace_back([]() {
				if (OpenSCAD::debug != "") PRINTD("glDisable(GL_CULL_FACE)");
				glDisable(GL_CULL_FACE);
			});
			halffacets_states.emplace_back(std::move(vs));
		}
		
		halffacets_data.createInterleavedVBO(halffacets_vbo);
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
	GLuint points_edges_vbo;
	GLuint halffacets_vbo;
	VertexStates points_edges_states;
	VertexStates halffacets_states;
}; // Polyhedron

} // namespace OGL

} //namespace CGAL
