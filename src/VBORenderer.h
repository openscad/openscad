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
	typedef struct _VertexSet
	{
		bool is_opencsg_vertex_set;
		OpenCSG::Operation operation;
		unsigned int convexity;
		GLsizei draw_size;
		GLintptr start_offset;
		bool draw_cull_front;
		bool draw_cull_back;
	} VertexSet;
	typedef struct _Vertex
	{
		GLfloat position[3];
		GLfloat normal[3];
		GLfloat color1[4];
		GLfloat color2[4];
		GLbyte trig[3];
		GLfloat pos_b[3];
		GLfloat pos_c[3];
		GLbyte mask[3];
	} Vertex;

	typedef std::pair<GLuint, std::vector<VertexSet *> *> VertexSets;

public:
	VBORenderer();
	virtual ~VBORenderer() {};
	virtual void resize(int w, int h);
	virtual bool getShaderColor(Renderer::ColorMode colormode, const Color4f &col, Color4f &outcolor) const;
	inline const GLint (&getVBOShaderSettings() const)[11] { return vbo_shader_settings; }

	virtual void create_surface(shared_ptr<const Geometry> geom, std::vector<Vertex> &render_buffer,
				    VertexSet &vertex_set, GLintptr prev_start_offset, GLsizei prev_draw_size,
				    csgmode_e csgmode, const Transform3d &m, const Color4f &color);
	virtual void draw_surface(const VertexSet &vertex_set, bool use_color_array = false, bool use_edge_shader = false) const;
	virtual inline void create_edges(shared_ptr<const Geometry> /* geom */, csgmode_e /* csgmode */) {}
	virtual inline void draw_edges(shared_ptr<const Geometry> geom, csgmode_e csgmode) const { render_edges(geom, csgmode); }

private:
	void create_triangle(std::vector<Vertex> &vertices, const Color4f &color,
			const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
			bool e0, bool e1, bool e2, bool mirrored) const;

	GLint vbo_shader_settings[11];
};

#endif // __VBORENDERER_H__
