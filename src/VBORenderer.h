#ifndef __VBORENDERER_H__
#define __VBORENDERER_H__

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgnode.h"
#include "VertexArray.h"

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

private:
	void create_triangle(VertexData &vertex_data, const Color4f &color,
				const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
				bool mirrored) const;

	shaderinfo_t vbo_renderer_shader;
};

#endif // __VBORENDERER_H__
