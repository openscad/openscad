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
	struct CGALVertexSet {
		GLuint vbo;
		std::unique_ptr<VBORenderer::VertexSet> vertex_set;
		
		CGALVertexSet(GLuint vbo_ = 0, std::unique_ptr<VBORenderer::VertexSet> vertex_set_ = nullptr)
			: vbo(vbo_), vertex_set(std::move(vertex_set_))
		{}
	};
	typedef std::vector<CGALVertexSet> CGALVertexSets;

	VBOPolyhedron()
		: Polyhedron()
	{
	}
	virtual ~VBOPolyhedron()
	{
		if (polyhedron_vertices_.vbo) {
			glDeleteBuffers(1, &polyhedron_vertices_.vbo);
		}
		if (polyhedron_edges_.vbo) {
			glDeleteBuffers(1, &polyhedron_edges_.vbo);
		}
		for (auto &halffacet : polyhedron_halffacets_) {
			if (halffacet.vbo) {
				glDeleteBuffers(1, &halffacet.vbo);
			}
		}
	}

	void draw(Vertex_iterator v, std::vector<VBORenderer::PCVertex> &render_buffer) const { 
		//      CGAL_NEF_TRACEN("drawing vertex "<<*v);
		PRINTD("draw(Vertex_iterator)");
		CGAL::Color c = getVertexColor(v);
		render_buffer.push_back({
			{(float)v->x(), (float)v->y(), (float)v->z()},
			{(float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0},
		});
	}

	void draw(Edge_iterator e, std::vector<VBORenderer::PCVertex> &render_buffer) const { 
		//      CGAL_NEF_TRACEN("drawing edge "<<*e);
		PRINTD("draw(Edge_iterator)");
		Double_point p = e->source(), q = e->target();
		CGAL::Color c = getEdgeColor(e);
		render_buffer.push_back({
			{(float)p.x(), (float)p.y(), (float)p.z()},
			{(float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0},
		});
		render_buffer.push_back({
			{(float)q.x(), (float)q.y(), (float)q.z()},
			{(float)c.red()/255.0f, (float)c.green()/255.0f, (float)c.blue()/255.0f, 1.0},
		});
	}

	typedef struct _TessUserData {
		GLdouble *normal;
		CGAL::Color color;
		bool cull_back;
		bool cull_front;
		std::vector<VBORenderer::PNCVertex> &render_buffer;
		CGALVertexSets &halffacets;
	} TessUserData;

	static inline void CGAL_GLU_TESS_CALLBACK beginCallback(GLenum which, GLvoid *user) {
		TessUserData *tess(static_cast<TessUserData *>(user));
		// Create separate vertex set since "which" could be different draw type
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		std::unique_ptr<VBORenderer::VertexSet> vertex_set(new VBORenderer::VertexSet({VBORenderer::PC, false, OpenSCADOperator::UNION, 0, which, 0, 0, tess->cull_front, tess->cull_back, false, false, false, 0}));
		tess->halffacets.emplace_back(vbo, std::move(vertex_set));
	}

	static inline void CGAL_GLU_TESS_CALLBACK endCallback(GLvoid *user) {
		TessUserData *tess(static_cast<TessUserData *>(user));
		
		glBufferData(GL_ARRAY_BUFFER, tess->render_buffer.size()*sizeof(VBORenderer::PNCVertex), tess->render_buffer.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		tess->halffacets.back().vertex_set->draw_size = tess->render_buffer.size();
		tess->render_buffer.clear();
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
		VBORenderer::PNCVertex render_vertex = {
			{(float)pc[0], (float)pc[1], (float)pc[2]},
			{(float)(tess->normal[0]), (float)(tess->normal[1]), (float)(tess->normal[2])},
			{(float)(tess->color.red()/255.0f), (float)(tess->color.green()/255.0f), (float)(tess->color.blue()/255.0f), 1.0}
		};
		tess->render_buffer.push_back(render_vertex);
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

	void draw(Halffacet_iterator f, std::vector<VBORenderer::PNCVertex> &render_buffer, bool is_back_facing) const {
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
			is_back_facing?true:false, is_back_facing?false:true,
			render_buffer,
			const_cast<CGALVertexSets &>(polyhedron_halffacets_)
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
		std::unique_ptr<std::vector<VBORenderer::PCVertex>> render_buffer = std::make_unique<std::vector<VBORenderer::PCVertex>>();
		PRINTD("create_polyhedron");
		GLuint vbo;
		
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// Vertices.
		Vertex_iterator v;
	        for(v=vertices_.begin();v!=vertices_.end();++v) 
			draw(v, *render_buffer);

		glBufferData(GL_ARRAY_BUFFER, render_buffer->size()*sizeof(VBORenderer::PCVertex), render_buffer->data(), GL_STATIC_DRAW);
		polyhedron_vertices_ = {vbo, std::unique_ptr<VBORenderer::VertexSet>(new VBORenderer::VertexSet({VBORenderer::PC, false, OpenSCADOperator::UNION, 0, GL_POINTS, (GLsizei)render_buffer->size(), 0, false, false, false, false, false, 0})) };

		if (render_buffer->size()) render_buffer->clear();
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// Edges
		Edge_iterator e;
	        for(e=edges_.begin();e!=edges_.end();++e)
			draw(e, *render_buffer);

		glBufferData(GL_ARRAY_BUFFER, render_buffer->size()*sizeof(VBORenderer::PCVertex), render_buffer->data(), GL_STATIC_DRAW);
		polyhedron_edges_ = {vbo, std::unique_ptr<VBORenderer::VertexSet>(new VBORenderer::VertexSet({VBORenderer::PC, false, OpenSCADOperator::UNION, 0, GL_LINES, (GLsizei)render_buffer->size(), 0, false, false, false, false, false, 0})) };

		std::unique_ptr<std::vector<VBORenderer::PNCVertex>> halffacet_buffer = std::make_unique<std::vector<VBORenderer::PNCVertex>>();
		for (int i = 0; i < (color_backfaces ? 2 : 1); i++) {
			// Halffacet
			Halffacet_iterator f;
		        for(f=halffacets_.begin();f!=halffacets_.end();++f)
				draw(f, *halffacet_buffer, i);
		}
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
	CGALVertexSet polyhedron_vertices_;
	CGALVertexSet polyhedron_edges_;
	CGALVertexSets polyhedron_halffacets_;

}; // Polyhedron

} // namespace OGL

} //namespace CGAL
