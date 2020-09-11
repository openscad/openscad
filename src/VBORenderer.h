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
	VBOShaderVertexState(size_t draw_offset) : VertexState(0,0,draw_offset) {}
	virtual ~VBOShaderVertexState() {}
};

class VBORenderer : public Renderer
{
public:
	VBORenderer();
	virtual ~VBORenderer() {};
	virtual void resize(int w, int h);
	virtual const Renderer::shaderinfo_t &getShader() const;
	virtual bool getShaderColor(Renderer::ColorMode colormode, const Color4f &col, Color4f &outcolor) const;

	virtual void create_surface(shared_ptr<const Geometry> geom, VertexArray &vertex_array,
				    csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;

	virtual void create_edges(shared_ptr<const Geometry> geom, VertexArray &vertex_array,
			  	  csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;

	virtual void create_polygons(shared_ptr<const Geometry> geom, VertexArray &vertex_array,
			  	     csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;
				     
protected:
	void add_shader_data(VertexArray &vertex_array) const;
	void add_shader_pointers(VertexArray &vertex_array) const;
	void shader_attribs_enable() const;
	void shader_attribs_disable() const;
	
private:
	void create_triangle(VertexArray &vertex_array, const Color4f &color,
				const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
				size_t primitive_index = 0,
				double z_offset = 0, size_t shape_size = 0,
				size_t shape_dimensions = 0, bool outlines = false,
				bool mirror = false) const;

	void add_shader_attributes(VertexArray &vertex_array, Color4f color,
				const std::vector<Vector3d> &points,
				size_t active_point_index = 0, size_t primitive_index = 0,
				double z_offset = 0, size_t shape_size = 0,
				size_t shape_dimensions = 0, bool outlines = false,
				bool mirror = false) const;

	shaderinfo_t vbo_renderer_shader;
	mutable size_t shader_write_index;
	enum ShaderAttribIndex {
		TRIG_ATTRIB,
		MASK_ATTRIB,
		POINT_B_ATTRIB,
		POINT_C_ATTRIB,
	};
};

#endif // __VBORENDERER_H__
