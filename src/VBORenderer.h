#ifndef __VBORENDERER_H__
#define __VBORENDERER_H__

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgnode.h"

class VBORenderer : public Renderer
{
public:
	struct VertexSet
	{
		bool is_opencsg_vertex_set;
		OpenSCADOperator operation;
		unsigned int convexity;
		GLenum draw_type;
		GLsizei draw_size;
		GLintptr start_offset;
		bool draw_cull_front;
		bool draw_cull_back;
		int identifier;
	};

	struct Vertex
	{
		GLfloat position[3];
		GLfloat normal[3];
		GLfloat color1[4];
		GLfloat color2[4];
		GLbyte trig[3];
		GLfloat pos_b[3];
		GLfloat pos_c[3];
		GLbyte mask[3];
	};

	typedef std::vector<std::unique_ptr<VertexSet>> VertexSets;
	typedef std::pair<GLuint, std::unique_ptr<VertexSets>> VBOVertexSets;
	typedef std::vector<VBOVertexSets> ProductVertexSets;

public:
	VBORenderer();
	virtual ~VBORenderer() {};
	virtual void resize(int w, int h);
	virtual const Renderer::shaderinfo_t &getShader() const;
	virtual bool getShaderColor(Renderer::ColorMode colormode, const Color4f &col, Color4f &outcolor) const;

	virtual void create_surface(shared_ptr<const Geometry> geom, std::vector<Vertex> &render_buffer,
				    VertexSet &vertex_set, GLintptr prev_start_offset, GLsizei prev_draw_size,
				    csgmode_e csgmode, const Transform3d &m, const Color4f &color) const;
	virtual void draw_surface(const VertexSet &vertex_set, const Renderer::shaderinfo_t *shaderinfo = nullptr, bool use_color_array = false) const;
	virtual inline void create_edges(shared_ptr<const Geometry> /* geom */, csgmode_e /* csgmode */) {}
	virtual inline void draw_edges(shared_ptr<const Geometry> geom, csgmode_e csgmode) const { render_edges(geom, csgmode); }

private:
	void create_triangle(std::vector<Vertex> &vertices, const Color4f &color,
			const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
			bool e0, bool e1, bool e2, bool mirrored) const;

	shaderinfo_t vbo_renderer_shader;
};

#endif // __VBORENDERER_H__
