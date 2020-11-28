#ifndef __VBORENDERER_H__
#define __VBORENDERER_H__

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgnode.h"
#include "VertexArray.h"

class VBOShaderVertexState : public VertexState
{
public:
	VBOShaderVertexState(size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo)
		: VertexState(0,0,0,draw_offset,element_offset,vertices_vbo,elements_vbo) {}
	virtual ~VBOShaderVertexState() {}
};

class VBORenderer : public Renderer
{
public:
	VBORenderer();
	virtual ~VBORenderer() {}
	virtual void resize(int w, int h);
	virtual bool getShaderColor(Renderer::ColorMode colormode, const Color4f &col, Color4f &outcolor) const;

	virtual void create_surface(const PolySet &ps, VertexArray &vertex_array,
				    csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;

	virtual void create_edges(const PolySet &ps, VertexArray &vertex_array,
			  	  csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;

	virtual void create_polygons(const PolySet &ps, VertexArray &vertex_array,
			  	     csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;

	virtual void create_triangle(VertexArray &vertex_array, const Color4f &color,
     				const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
     				size_t primitive_index = 0,
     				double z_offset = 0, size_t shape_size = 0,
     				size_t shape_dimensions = 0, bool outlines = false,
     				bool mirror = false) const;

	virtual void create_vertex(VertexArray &vertex_array, const Color4f &color,
     				const std::array<Vector3d,3> &points,
     				const std::array<Vector3d,3> &normals,
     				size_t active_point_index = 0, size_t primitive_index = 0,
     				double z_offset = 0, size_t shape_size = 0,
     				size_t shape_dimensions = 0, bool outlines = false,
     				bool mirror = false) const;
				     
protected:
	void add_shader_data(VertexArray &vertex_array) const;
	void add_shader_pointers(VertexArray &vertex_array) const;
	void shader_attribs_enable() const;
	void shader_attribs_disable() const;
	
private:
	void add_shader_attributes(VertexArray &vertex_array, Color4f color,
				const std::array<Vector3d,3> &points,
				const std::array<Vector3d,3> &normals,
				size_t active_point_index = 0, size_t primitive_index = 0,
				double z_offset = 0, size_t shape_size = 0,
				size_t shape_dimensions = 0, bool outlines = false,
				bool mirror = false) const;

	mutable size_t shader_write_index;
	enum ShaderAttribIndex {
		BARYCENTRIC_ATTRIB
	};
};

#endif // __VBORENDERER_H__
