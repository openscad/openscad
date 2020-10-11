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

#include "VBORenderer.h"
#include "feature.h"
#include "polyset.h"
#include "csgnode.h"
#include "printutils.h"

#include <cstddef>

VBORenderer::VBORenderer()
	: Renderer(), shader_write_index(0)
{
	vbo_renderer_shader.progid = 0;
	
	const char *vs_source = R"(
		uniform float xscale, yscale;
		attribute vec4 color_area;
		attribute vec3 trig, mask;
		attribute vec3 point_b, point_c;
		varying vec4 fcolor_area;
		varying vec3 tp, tr;
		varying float shading;
		void main() {
			vec4 p0 = gl_ModelViewProjectionMatrix * gl_Vertex;
			vec4 p1 = gl_ModelViewProjectionMatrix * vec4(point_b, 1.0);
			vec4 p2 = gl_ModelViewProjectionMatrix * vec4(point_c, 1.0);
			float a = distance(vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));
			float b = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w));
			float c = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));
			float s = (a + b + c) / 2.0;
			float A = sqrt(s*(s-a)*(s-b)*(s-c));
			float ha = 2.0*A/a;
			gl_Position = p0;
			tp = vec3(float(mask.x), float(mask.y), float(mask.z)) * ha;
			tr = vec3(float(trig.x), float(trig.y), float(trig.z));
			vec3 normal, lightDir;
			normal = normalize(gl_NormalMatrix * gl_Normal);
			lightDir = normalize(vec3(gl_LightSource[0].position));
			shading = 0.2 + abs(dot(normal, lightDir));
			fcolor_area = color_area;
		}
	)";

	/*
	Inputs:
	tp && tr - if any components of tp < tr, use color2 (edge color)
	shading  - multiplied by color1. color2 is is without lighting
	*/
	const char *fs_source = R"(
		varying vec4 fcolor_area;
		varying vec3 tp, tr, tmp;
		varying float shading;
		void main() {
			gl_FragColor = vec4(fcolor_area.r * shading, fcolor_area.g * shading, fcolor_area.b * shading, fcolor_area.a);
			if (tp.x < tr.x || tp.y < tr.y || tp.z < tr.z)
				gl_FragColor = vec4((fcolor_area.r+1.0)/2.0, (fcolor_area.g+1.0)/2.0, (fcolor_area.b+1.0)/2.0, 1.0);
		}
	)";
	GLint status;
	GLenum err;
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);
	glCompileShader(vs);
	err = glGetError();
	if (err != GL_NO_ERROR) {
		PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
		return;
	}
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		int loglen;
		char logbuffer[1000];
		glGetShaderInfoLog(vs, sizeof(logbuffer), &loglen, logbuffer);
		PRINTDB("OpenGL Program Compile Vertex Shader Error:\n%s", logbuffer);
		return;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);
	glCompileShader(fs);
	err = glGetError();
	if (err != GL_NO_ERROR) {
		PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
		return;
	}
	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		int loglen;
		char logbuffer[1000];
		glGetShaderInfoLog(fs, sizeof(logbuffer), &loglen, logbuffer);
		PRINTDB("OpenGL Program Compile Fragement Shader Error:\n%s", logbuffer);
		return;
	}

	GLuint vbo_shader_prog = glCreateProgram();
	glAttachShader(vbo_shader_prog, vs);
	glAttachShader(vbo_shader_prog, fs);
	glLinkProgram(vbo_shader_prog);

	err = glGetError();
	if (err != GL_NO_ERROR) {
		PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
		return;
	}

	glGetProgramiv(vbo_shader_prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		int loglen;
		char logbuffer[1000];
		glGetProgramInfoLog(vbo_shader_prog, sizeof(logbuffer), &loglen, logbuffer);
		PRINTDB("OpenGL Program Linker Error:\n%s", logbuffer);
		return;
	}
	
	int loglen;
	char logbuffer[1000];
	glGetProgramInfoLog(vbo_shader_prog, sizeof(logbuffer), &loglen, logbuffer);
	if (loglen > 0) {
		PRINTDB("OpenGL Program Link OK:\n%s", logbuffer);
	}
	glValidateProgram(vbo_shader_prog);
	glGetProgramInfoLog(vbo_shader_prog, sizeof(logbuffer), &loglen, logbuffer);
	if (loglen > 0) {
		PRINTDB("OpenGL Program Validation results:\n%s", logbuffer);
	}

	vbo_renderer_shader.progid = vbo_shader_prog; // 0
	vbo_renderer_shader.type = EDGE_RENDERING;
	vbo_renderer_shader.data.csg_rendering.color_area = glGetAttribLocation(vbo_shader_prog, "color_area");
	vbo_renderer_shader.data.csg_rendering.trig = glGetAttribLocation(vbo_shader_prog, "trig");
	vbo_renderer_shader.data.csg_rendering.mask = glGetAttribLocation(vbo_shader_prog, "mask");
	vbo_renderer_shader.data.csg_rendering.point_b = glGetAttribLocation(vbo_shader_prog, "point_b");
	vbo_renderer_shader.data.csg_rendering.point_c = glGetAttribLocation(vbo_shader_prog, "point_c");
	vbo_renderer_shader.data.csg_rendering.xscale = glGetUniformLocation(vbo_shader_prog, "xscale");
	vbo_renderer_shader.data.csg_rendering.yscale = glGetUniformLocation(vbo_shader_prog, "yscale");
	PRINTDB("vbo_renderer_shader color_area = %d", vbo_renderer_shader.data.csg_rendering.color_area);
	PRINTDB("vbo_renderer_shader trig = %d", vbo_renderer_shader.data.csg_rendering.trig);
	PRINTDB("vbo_renderer_shader mask = %d", vbo_renderer_shader.data.csg_rendering.mask);
	PRINTDB("vbo_renderer_shader point_b = %d", vbo_renderer_shader.data.csg_rendering.point_b);
	PRINTDB("vbo_renderer_shader point_c = %d", vbo_renderer_shader.data.csg_rendering.point_c);
	PRINTDB("vbo_renderer_shader xscale = %d", vbo_renderer_shader.data.csg_rendering.xscale);
	PRINTDB("vbo_renderer_shader yscale = %d", vbo_renderer_shader.data.csg_rendering.yscale);
}

void VBORenderer::resize(int w, int h)
{
	Renderer::resize(w,h);
	vbo_renderer_shader.vp_size_x = w;
	vbo_renderer_shader.vp_size_y = h;
}

const Renderer::shaderinfo_t &VBORenderer::getShader() const
{
	if (!Feature::ExperimentalVxORenderers.is_enabled()) {
		return Renderer::getShader();
	}
	return vbo_renderer_shader;
}

bool VBORenderer::getShaderColor(Renderer::ColorMode colormode, const Color4f &color, Color4f &outcolor) const
{
	Color4f basecol;
	if (Renderer::getColor(colormode, basecol)) {
		if (colormode == ColorMode::BACKGROUND) {
			basecol = Color4f(color[0] >= 0 ? color[0] : basecol[0],
					color[1] >= 0 ? color[1] : basecol[1],
					color[2] >= 0 ? color[2] : basecol[2],
					color[3] >= 0 ? color[3] : basecol[3]);
		}
		else if (colormode != ColorMode::HIGHLIGHT) {
			basecol = Color4f(color[0] >= 0 ? color[0] : basecol[0],
					color[1] >= 0 ? color[1] : basecol[1],
					color[2] >= 0 ? color[2] : basecol[2],
					color[3] >= 0 ? color[3] : basecol[3]);
		}
		Color4f col;
		Renderer::getColor(ColorMode::MATERIAL, col);
		outcolor = basecol;
		if (outcolor[0] < 0) outcolor[0] = col[0];
		if (outcolor[1] < 0) outcolor[1] = col[1];
		if (outcolor[2] < 0) outcolor[2] = col[2];
		if (outcolor[3] < 0) outcolor[3] = col[3];
		return true;
	}

	return false;
}

void VBORenderer::add_shader_attributes(VertexArray &vertex_array, Color4f color,
					const std::vector<Vector3d> &points,
					size_t active_point_index, size_t primitive_index,
					double z_offset, size_t shape_size,
					size_t shape_dimensions, bool outlines,
					bool mirror) const
{
	if (!vertex_array.data(shader_write_index)) return;
	
	shared_ptr<VertexData> vertex_data = vertex_array.data(shader_write_index);

	if (OpenSCAD::debug != "") PRINTDB("create_vertex(%d, %d, %f, %d, %d, %d, %d)",
		active_point_index % primitive_index % z_offset % shape_size % shape_dimensions % outlines % mirror);
	if (points.size() == 3 &&
		vbo_renderer_shader.data.csg_rendering.trig && vbo_renderer_shader.data.csg_rendering.mask &&
		vbo_renderer_shader.data.csg_rendering.point_b && vbo_renderer_shader.data.csg_rendering.point_c) {
		// Get edge states
		std::array<GLshort, 3> trig_flags;
		std::array<GLshort, 3> mask_flags;
		if (!outlines) {
			// top / bottom or 3d object
			if (shape_size == 3) {
				trig_flags = {2, 2, 2};
			} else if (shape_size == 4) {
				trig_flags = {2, -1, 2};
			} else {
				trig_flags = {-1, 2, -1};
			}
		} else {
			// sides
			if (primitive_index == 0) {
				trig_flags = {2, 2, -1};
			} else {
				trig_flags = {-1, 2, 2};
			}
		}
		
		Vector3d pb, pc;
	
		if (active_point_index == 0) {
			mask_flags = {0, 1, 0};
			pb = points[1];
			pc = points[2];
		}
		if (active_point_index == 1) {
			mask_flags = {0, 0, 1};
			pb = points[0];
			pc = points[2];
		}
		if (active_point_index == 2) {
			mask_flags = {1, 0, 0};
			pb = points[0];
			pc = points[1];
		}

		addAttributeValues(*(vertex_data->attributes()[TRIG_ATTRIB]), trig_flags[0], trig_flags[1], trig_flags[2]);
		addAttributeValues(*(vertex_data->attributes()[MASK_ATTRIB]), mask_flags[0], mask_flags[1], mask_flags[2]);
		addAttributeValues(*(vertex_data->attributes()[POINT_B_ATTRIB]), pb[0], pb[1], pb[2]);
		addAttributeValues(*(vertex_data->attributes()[POINT_C_ATTRIB]), pc[0], pc[1], pc[2]);

		if (OpenSCAD::debug != "") PRINTDB("create_vertex trig : [%d, %d, %d]",
			trig_flags[0] % trig_flags[1] % trig_flags[2]);
		if (OpenSCAD::debug != "") PRINTDB("create_vertex mask : [%d, %d, %d]",
			mask_flags[0] % mask_flags[1] % mask_flags[2]);
		if (OpenSCAD::debug != "") PRINTDB("create_vertex point_b : [%d, %d, %d]",
			pb[0] % pb[1] % (pb[2]));
		if (OpenSCAD::debug != "") PRINTDB("create_vertex point_c : [%d, %d, %d]",
			pc[0] % pc[1] % (pc[2]));

	} else {
		if (OpenSCAD::debug != "") PRINTDB("create_vertex bad points size = %d", points.size());
	}
}

void VBORenderer::create_triangle(VertexArray &vertex_array, const Color4f &color,
				const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
				size_t primitive_index,
				double z_offset, size_t shape_size,
				size_t shape_dimensions, bool outlines,
				bool mirror) const
{
	double ax = p1[0] - p0[0], bx = p1[0] - p2[0];
	double ay = p1[1] - p0[1], by = p1[1] - p2[1];
	double az = p1[2] - p0[2], bz = p1[2] - p2[2];
	double nx = ay*bz - az*by;
	double ny = az*bx - ax*bz;
	double nz = ax*by - ay*bx;
	double nl = sqrt(nx*nx + ny*ny + nz*nz);
	
	if (!vertex_array.data()) return;
	
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (shader_write_index)
		add_shader_attributes(vertex_array, color, {p0, p1, p2}, 0,
		      			primitive_index, z_offset, shape_size,
					shape_dimensions, outlines, mirror);
	addAttributeValues(*(vertex_data->positionData()), p0[0], p0[1], p0[2]);
	if (!mirror) {
		if (shader_write_index)
			add_shader_attributes(vertex_array, color, {p0, p1, p2}, 1,
						primitive_index, z_offset, shape_size,
						shape_dimensions, outlines, mirror);
		addAttributeValues(*(vertex_data->positionData()), p1[0], p1[1], p1[2]);
	}
	if (shader_write_index)
		add_shader_attributes(vertex_array, color, {p0, p1, p2}, 2,
					primitive_index, z_offset, shape_size,
					shape_dimensions, outlines, mirror);
	addAttributeValues(*(vertex_data->positionData()), p2[0], p2[1], p2[2]);
	if (mirror) {
		if (shader_write_index)
			add_shader_attributes(vertex_array, color, {p0, p1, p2}, 1,
						primitive_index, z_offset, shape_size,
						shape_dimensions, outlines, mirror);
		addAttributeValues(*(vertex_data->positionData()), p1[0], p1[1], p1[2]);
	}
	if (vertex_data->hasNormalData()) {
		addAttributeValues(3, *(vertex_data->normalData()), (nx/nl), (ny/nl), (nz/nl));		
	}
	if (vertex_data->hasColorData()) {
		addAttributeValues(3, *(vertex_data->colorData()), color[0], color[1], color[2], color[3]);
	}
}

void VBORenderer::create_surface(const Geometry *geom, VertexArray &vertex_array,
				 csgmode_e csgmode, const Transform3d &m, const Color4f &color) const
{
	const PolySet* ps = dynamic_cast<const PolySet*>(geom);
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!ps || !vertex_data) { return; }

	bool mirrored = m.matrix().determinant() < 0;
	size_t triangle_count = 0;

	if (ps->getDimension() == 2) {
		create_polygons(geom, vertex_array, csgmode, m, color);
	} else if (ps->getDimension() == 3) {
		VertexStates &vertex_states = vertex_array.states();
		size_t last_size = vertex_data->sizeInBytes();

		for (const auto &poly : ps->polygons) {
			Vector3d p0 = m * poly.at(0);
			Vector3d p1 = m * poly.at(1);
			Vector3d p2 = m * poly.at(2);

			if (poly.size() == 3) {
				create_triangle(vertex_array, color, p0, p1, p2,
					0, 0, poly.size(), 3, false, mirrored);
				triangle_count++;
			}
			else if (poly.size() == 4) {
				Vector3d p3 = m * poly.at(3);
				create_triangle(vertex_array, color, p0, p1, p3,
					0, 0, poly.size(), 3, false, mirrored);
				create_triangle(vertex_array, color, p2, p3, p1,
					1, 0, poly.size(), 3, false, mirrored);
				triangle_count+=2;
			}
			else {
				Vector3d center = Vector3d::Zero();
				for (size_t i = 0; i < poly.size(); i++) {
					center += poly.at(i);
				}
				center /= poly.size();
				for (size_t i = 1; i <= poly.size(); i++) {
					Vector3d p0 = m * center;
					Vector3d p1 = m * poly.at(i % poly.size());
					Vector3d p2 = m * poly.at(i - 1);
					create_triangle(vertex_array, color, p0, p2, p1,
						i-1, 0, poly.size(), 3, false, mirrored);
					triangle_count++;
				}
			}
		}

		std::shared_ptr<VertexState> vs = vertex_array.createVertexState(GL_TRIANGLES, triangle_count*3, vertex_array.writeIndex());
		vertex_states.emplace_back(std::move(vs));
		vertex_array.addAttributePointers(last_size);
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
}

void VBORenderer::create_edges(const Geometry *geom,
				VertexArray &vertex_array, csgmode_e csgmode,
				const Transform3d &m,
				const Color4f &color) const
{
	const PolySet* ps = dynamic_cast<const PolySet*>(geom);
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!ps || !vertex_data) return;

	VertexStates &vertex_states = vertex_array.states();

	if (ps->getDimension() == 2) {
		if (csgmode == Renderer::CSGMODE_NONE) {
			// Render only outlines
			for (const Outline2d &o : ps->getPolygon().outlines()) {
				size_t last_size = vertex_data->sizeInBytes();
				for (const Vector2d &v : o.vertices) {
					Vector3d p0(v[0],v[1],0.0); p0 = m * p0;

					addAttributeValues(*(vertex_data->positionData()), (float)p0[0], (float)p0[1], (float)p0[2]);
					if (vertex_data->hasColorData()) {
						addAttributeValues(*(vertex_data->colorData()), color[0], color[1], color[2], color[3]);
					}
				}
				std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(GL_LINE_LOOP, o.vertices.size(), vertex_array.writeIndex());
				vertex_states.emplace_back(std::move(line_loop));
				vertex_array.addAttributePointers(last_size);
			}
		} else {
			// Render 2D objects 1mm thick, but differences slightly larger
			double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0.0);
			for (const Outline2d &o : ps->getPolygon().outlines()) {
				size_t last_size = vertex_data->sizeInBytes();

				// Render top+bottom outlines
				for (double z = -zbase/2; z < zbase; z += zbase) {
					for (const Vector2d &v : o.vertices) {
						Vector3d p0(v[0],v[1],z); p0 = m * p0;

						addAttributeValues(*(vertex_data->positionData()), (float)p0[0], (float)p0[1], (float)p0[2]);
						if (vertex_data->hasColorData()) {
							PRINTD("create_edges adding color to top/bottom outline");
							addAttributeValues(*(vertex_data->colorData()), color[0], color[1], color[2], color[3]);
						}
					}
				}

				std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(GL_LINE_LOOP, o.vertices.size()*2, vertex_array.writeIndex());
				vertex_states.emplace_back(std::move(line_loop));
				vertex_array.addAttributePointers(last_size);

				last_size = vertex_data->sizeInBytes();
				// Render sides
				for (const Vector2d &v : o.vertices) {
					Vector3d p0(v[0], v[1], -zbase/2); p0 = m * p0;
					Vector3d p1(v[0], v[1], +zbase/2); p1 = m * p1;
					addAttributeValues(*(vertex_data->positionData()), (float)p0[0], (float)p0[1], (float)p0[2]);
					addAttributeValues(*(vertex_data->positionData()), (float)p1[0], (float)p1[1], (float)p1[2]);
					if (vertex_data->hasColorData()) {
						PRINTD("create_edges adding color to outline sides");
						addAttributeValues(2, *(vertex_data->colorData()), color[0], color[1], color[2], color[3]);
					}
				}
				
				std::shared_ptr<VertexState> lines = vertex_array.createVertexState(GL_LINES, o.vertices.size()*2, vertex_array.writeIndex());
				vertex_states.emplace_back(std::move(lines));
				vertex_array.addAttributePointers(last_size);
			}
		}
	} else if (ps->getDimension() == 3) {
		for (const auto &polygon : ps->polygons) {
			size_t last_size = vertex_data->sizeInBytes();
			for (const auto &vertex : polygon) {
				const Vector3d &p = m * vertex;
				addAttributeValues(*(vertex_data->positionData()), (float)p[0], (float)p[1], (float)p[2]);
				if (vertex_data->hasColorData()) {
					addAttributeValues(*(vertex_data->colorData()), color[0], color[1], color[2], color[3]);
				}
			}
			std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(GL_LINE_LOOP, polygon.size(), vertex_array.writeIndex());
			vertex_states.emplace_back(std::move(line_loop));
			vertex_array.addAttributePointers(last_size);
		}
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
}

void VBORenderer::create_polygons(const Geometry *geom, VertexArray &vertex_array,
				  csgmode_e csgmode, const Transform3d &m, const Color4f &color) const
{
	const PolySet* ps = dynamic_cast<const PolySet*>(geom);
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!ps || !vertex_data) return;

	VertexStates &vertex_states = vertex_array.states();

	if (ps->getDimension() == 2) {
		PRINTD("create_polygons 2D");
		bool mirrored = m.matrix().determinant() < 0;
		size_t triangle_count = 0;
		size_t last_size = vertex_data->sizeInBytes();

		if (csgmode == Renderer::CSGMODE_NONE) {
			PRINTD("create_polygons CSGMODE_NONE");
			for (const auto &poly : ps->polygons) {
				Vector3d p0 = poly.at(0); p0 = m * p0;
				Vector3d p1 = poly.at(1); p1 = m * p1;
				Vector3d p2 = poly.at(2); p2 = m * p2;

				if (poly.size() == 3) {
					create_triangle(vertex_array, color, p0, p1, p2,
						0, 0, poly.size(), 2, false, mirrored);
					triangle_count++;
				}
				else if (poly.size() == 4) {
					Vector3d p3 = poly.at(3); p3 = m * p3;

					create_triangle(vertex_array, color, p0, p1, p3,
						0, 0, poly.size(), 2, false, mirrored);
					create_triangle(vertex_array, color, p2, p3, p1,
						1, 0, poly.size(), 2, false, mirrored);
					triangle_count+=2;
				}
				else {
					Vector3d center = Vector3d::Zero();
					for (const auto &point : poly) {
						center[0] += point[0];
						center[1] += point[1];
					}
					center[0] /= poly.size();
					center[1] /= poly.size();

					for (size_t i = 1; i <= poly.size(); i++) {
						Vector3d p0 = center; p0 = m * p0;
						Vector3d p1 = poly.at(i % poly.size()); p1 = m * p1;
						Vector3d p2 = poly.at(i - 1); p2 = m * p2;

						create_triangle(vertex_array, color, p0, p2, p1,
							i-1, 0, poly.size(), 2, false, mirrored);
						triangle_count++;
					}
				}
			}
		} else {
			PRINTD("create_polygons 1mm thick");
			// Render 2D objects 1mm thick, but differences slightly larger
			double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0.0);
			// Render top+bottom
			for (double z = -zbase/2; z < zbase; z += zbase) {
				for (const auto &poly : ps->polygons) {
					Vector3d p0 = poly.at(0); p0[2] += z; p0 = m * p0;
					Vector3d p1 = poly.at(1); p1[2] += z; p1 = m * p1;
					Vector3d p2 = poly.at(2); p2[2] += z; p2 = m * p2;

					if (poly.size() == 3) {
						if (z < 0) {
							create_triangle(vertex_array, color, p0, p2, p1,
								0, z, poly.size(), 2, false, mirrored);
						} else {
							create_triangle(vertex_array, color, p0, p1, p2,
								0, z, poly.size(), 2, false, mirrored);
						}
						triangle_count++;
					}
					else if (poly.size() == 4) {
						Vector3d p3 = poly.at(3); p3[2] += z; p3 = m * p3;
						if (z < 0) {
							create_triangle(vertex_array, color, p0, p3, p1,
								0, z, poly.size(), 2, false, mirrored);
							create_triangle(vertex_array, color, p2, p1, p3,
								1, z, poly.size(), 2, false, mirrored);
						} else {
							create_triangle(vertex_array, color, p0, p1, p3,
								0, z, poly.size(), 2, false, mirrored);
							create_triangle(vertex_array, color, p2, p3, p1,
								1, z, poly.size(), 2, false, mirrored);
						}
						triangle_count+=2;
					}
					else {
						Vector3d center = Vector3d::Zero();
						for (const auto &point : poly) {
							center[0] += point[0];
							center[1] += point[1];
						}
						center[0] /= poly.size();
						center[1] /= poly.size();

						for (size_t i = 1; i <= poly.size(); i++) {
							Vector3d p0 = center; center[2] += z; p0 = m * p0;
							Vector3d p1 = poly.at(i % poly.size()); p1[2] += z; p1 = m * p1;
							Vector3d p2 = poly.at(i - 1); p2[2] += z; p2 = m * p2;

							if (z < 0) {
								create_triangle(vertex_array, color, p0, p1, p2,
									i-1, z, poly.size(), 2, false, mirrored);
							} else {
								create_triangle(vertex_array, color, p0, p2, p1,
									i-1, z, poly.size(), 2, false, mirrored);
							}
							triangle_count++;
						}
					}
				}
			}

			// Render sides
			if (ps->getPolygon().outlines().size() > 0) {
				PRINTD("Render outlines as sides");
				for (const Outline2d &o : ps->getPolygon().outlines()) {
					for (size_t i = 1; i <= o.vertices.size(); i++) {
						Vector3d p1 = m * Vector3d(o.vertices[i-1][0], o.vertices[i-1][1], -zbase/2);
						Vector3d p2 = m * Vector3d(o.vertices[i-1][0], o.vertices[i-1][1], zbase/2);
						Vector3d p3 = m * Vector3d(o.vertices[i % o.vertices.size()][0], o.vertices[i % o.vertices.size()][1], -zbase/2);
						Vector3d p4 = m * Vector3d(o.vertices[i % o.vertices.size()][0], o.vertices[i % o.vertices.size()][1], zbase/2);
						
						create_triangle(vertex_array, color, p2, p1, p3,
							0, 0, o.vertices.size(), 2, true, mirrored);
						create_triangle(vertex_array, color, p2, p3, p4,
							1, 0, o.vertices.size(), 2, true, mirrored);
						triangle_count+=2;
					}
				}
			} else {
				// If we don't have borders, use the polygons as borders.
				// FIXME: When is this used?
				PRINTD("Render sides with polygons");
				for (const auto &poly : ps->polygons) {
					for (size_t i = 1; i <= poly.size(); i++) {
						Vector3d p1 = poly.at(i - 1); p1[2] -= zbase/2; p1 = m * p1;
						Vector3d p2 = poly.at(i - 1); p2[2] += zbase/2; p2 = m * p2;
						Vector3d p3 = poly.at(i % poly.size()); p3[2] -= zbase/2; p3 = m * p3;
						Vector3d p4 = poly.at(i % poly.size()); p4[2] += zbase/2; p4 = m * p4;

						create_triangle(vertex_array, color, p2, p1, p3,
							0, 0, poly.size(), 2, true, mirrored);
						create_triangle(vertex_array, color, p2, p3, p4,
							1, 0, poly.size(), 2, true, mirrored);
						triangle_count+=2;
					}
				}
			}
		}
		
		std::shared_ptr<VertexState> vs = vertex_array.createVertexState(GL_TRIANGLES, triangle_count*3, vertex_array.writeIndex());
		vertex_states.emplace_back(std::move(vs));
		vertex_array.addAttributePointers(last_size);
	} else {
		assert(false && "Cannot render object with no dimension");
	}
}

void VBORenderer::add_shader_data(VertexArray &vertex_array) const
{
	std::shared_ptr<VertexData> shader_data = std::make_shared<VertexData>();
	shader_data->addAttributeData(std::make_shared<AttributeData<GLshort,3,GL_SHORT>>()); // trig
	shader_data->addAttributeData(std::make_shared<AttributeData<GLshort,3,GL_SHORT>>()); // mask
	shader_data->addAttributeData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>()); // point_b
	shader_data->addAttributeData(std::make_shared<AttributeData<GLfloat,3,GL_FLOAT>>()); // point_c
	shader_write_index = vertex_array.size();
	vertex_array.addVertexData(shader_data);
}

void VBORenderer::add_shader_pointers(VertexArray &vertex_array) const
{
	shared_ptr<VertexData> vertex_data = vertex_array.data(); // current data
	shared_ptr<VertexData> shader_data = vertex_array.data(shader_write_index);

	if (!vertex_data || !shader_data) return;
	
	size_t vertex_start_offset = vertex_data->sizeInBytes();
	size_t shader_start_offset = shader_data->sizeInBytes();

	std::shared_ptr<VertexState> vs = std::make_shared<VBOShaderVertexState>(vertex_array.writeIndex());
	std::shared_ptr<VertexState> ss = std::make_shared<VBOShaderVertexState>(shader_write_index);
	GLuint index = 0;
	GLsizei count = 0, stride = 0;
	GLenum type = 0;
	size_t offset = 0;

	if (vertex_data->hasColorData() && vbo_renderer_shader.data.csg_rendering.color_area) {
		index = vbo_renderer_shader.data.csg_rendering.color_area;
		count = vertex_data->colorData()->count();
		type = vertex_data->colorData()->glType();
		stride = vertex_data->stride();
		offset = vertex_start_offset + vertex_data->interleavedOffset(vertex_data->colorIndex());
		vs->glBegin().emplace_back([index, count, type, stride, offset, vs_ptr = std::weak_ptr<VertexState>(vs)]() {
			auto vs = vs_ptr.lock();
			if (vs) {
				if (OpenSCAD::debug != "") PRINTDB("glVertexAttribPointer(%d, %d, %d, %p)",
					count % type % stride % (GLvoid *)(vs->drawOffset() + offset));
				glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(vs->drawOffset() + offset));
			}
		});
	}

	if (vbo_renderer_shader.data.csg_rendering.trig) {
		index = vbo_renderer_shader.data.csg_rendering.trig;
		count = shader_data->attributes()[TRIG_ATTRIB]->count();
		type = shader_data->attributes()[TRIG_ATTRIB]->glType();
		stride = shader_data->stride();
		offset = shader_start_offset + shader_data->interleavedOffset(TRIG_ATTRIB);
		ss->glBegin().emplace_back([index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
			auto ss = ss_ptr.lock();
			if (ss) {
				if (OpenSCAD::debug != "") PRINTDB("glVertexAttribPointer(%d, %d, %d, %p)",
					count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
				glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(ss->drawOffset() + offset));
			}
		});
	}

	if (vbo_renderer_shader.data.csg_rendering.mask) {
		index = vbo_renderer_shader.data.csg_rendering.mask;
		count = shader_data->attributes()[MASK_ATTRIB]->count();
		type = shader_data->attributes()[MASK_ATTRIB]->glType();
		stride = shader_data->stride();
		offset = shader_start_offset + shader_data->interleavedOffset(MASK_ATTRIB);
		ss->glBegin().emplace_back([index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
			auto ss = ss_ptr.lock();
			if (ss) {
				if (OpenSCAD::debug != "") PRINTDB("glVertexAttribPointer(%d, %d, %d, %p)",
					count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
				glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(ss->drawOffset() + offset));
			}
		});
	}

	if (vbo_renderer_shader.data.csg_rendering.point_b) {
		index = vbo_renderer_shader.data.csg_rendering.point_b;
		count = shader_data->attributes()[POINT_B_ATTRIB]->count();
		type = shader_data->attributes()[POINT_B_ATTRIB]->glType();
		stride = shader_data->stride();
		offset = shader_start_offset + shader_data->interleavedOffset(POINT_B_ATTRIB);
		ss->glBegin().emplace_back([index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
			auto ss = ss_ptr.lock();
			if (ss) {
				if (OpenSCAD::debug != "") PRINTDB("glVertexAttribPointer(%d, %d, %d, %p)",
					count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
				glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(ss->drawOffset() + offset));
			}
		});
	}

	if (vbo_renderer_shader.data.csg_rendering.point_c) {
		index = vbo_renderer_shader.data.csg_rendering.point_c;
		count = shader_data->attributes()[POINT_C_ATTRIB]->count();
		type = shader_data->attributes()[POINT_C_ATTRIB]->glType();
		stride = shader_data->stride();
		offset = shader_start_offset + shader_data->interleavedOffset(POINT_C_ATTRIB);
		ss->glBegin().emplace_back([index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
			auto ss = ss_ptr.lock();
			if (ss) {
				if (OpenSCAD::debug != "") PRINTDB("glVertexAttribPointer(%d, %d, %d, %p)",
					count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
				glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(ss->drawOffset() + offset));
			}
		});
	}

	vertex_array.states().emplace_back(std::move(vs));
	vertex_array.states().emplace_back(std::move(ss));
}

void VBORenderer::shader_attribs_enable() const
{
	if (OpenSCAD::debug != "") PRINTDB("glEnableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.color_area);
	glEnableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.color_area);

	if (OpenSCAD::debug != "") PRINTDB("glEnableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.trig);
	glEnableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.trig);

	if (OpenSCAD::debug != "") PRINTDB("glEnableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.mask);
	glEnableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.mask);
	
	if (OpenSCAD::debug != "") PRINTDB("glEnableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.point_b);
	glEnableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.point_b);

	if (OpenSCAD::debug != "") PRINTDB("glEnableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.point_c);
	glEnableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.point_c);
}
void VBORenderer::shader_attribs_disable() const
{
	if (OpenSCAD::debug != "") PRINTDB("glDisableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.color_area);
	glDisableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.color_area);

	if (OpenSCAD::debug != "") PRINTDB("glDisableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.trig);
	glDisableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.trig);

	if (OpenSCAD::debug != "") PRINTDB("glDisableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.mask);
	glDisableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.mask);
	
	if (OpenSCAD::debug != "") PRINTDB("glDisableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.point_b);
	glDisableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.point_b);

	if (OpenSCAD::debug != "") PRINTDB("glDisableVertexAttribArray(%d)", vbo_renderer_shader.data.csg_rendering.point_c);
	glDisableVertexAttribArray(vbo_renderer_shader.data.csg_rendering.point_c);
}