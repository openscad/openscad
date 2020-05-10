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
#include "polyset.h"
#include "csgnode.h"
#include "printutils.h"

#include <cstddef>

VBORenderer::VBORenderer()
	: Renderer()
{
    const char *vs_source =
    "uniform float xscale, yscale;\n"
    "attribute vec4 color1, color2;\n"
    "attribute vec3 pos_b, pos_c;\n"
    "attribute vec3 trig, mask;\n"
    "varying vec4 fcolor1, fcolor2;\n"
    "varying vec3 tp, tr;\n"
    "varying float shading;\n"
    "void main() {\n"
    "  vec4 p0 = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "  vec4 p1 = gl_ModelViewProjectionMatrix * vec4(pos_b, 1.0);\n"
    "  vec4 p2 = gl_ModelViewProjectionMatrix * vec4(pos_c, 1.0);\n"
    "  float a = distance(vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
    "  float b = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w));\n"
    "  float c = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
    "  float s = (a + b + c) / 2.0;\n"
    "  float A = sqrt(s*(s-a)*(s-b)*(s-c));\n"
    "  float ha = 2.0*A/a;\n"
    "  gl_Position = p0;\n"
    "  tp = mask * ha;\n"
    "  tr = trig;\n"
    "  vec3 normal, lightDir;\n"
    "  normal = normalize(gl_NormalMatrix * gl_Normal);\n"
    "  lightDir = normalize(vec3(gl_LightSource[0].position));\n"
    "  shading = 0.2 + abs(dot(normal, lightDir));\n"
    "  fcolor1 = color1;\n"
    "  fcolor2 = color2;\n"
    "}\n";

    /*
    Inputs:
    tp && tr - if any components of tp < tr, use color2 (edge color)
    shading  - multiplied by color1. color2 is is without lighting
    */
    const char *fs_source =
    "varying vec4 fcolor1, fcolor2;\n"
    "varying vec3 tp, tr, tmp;\n"
    "varying float shading;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(fcolor1.r * shading, fcolor1.g * shading, fcolor1.b * shading, fcolor1.a);\n"
    "  if (tp.x < tr.x || tp.y < tr.y || tp.z < tr.z)\n"
    "    gl_FragColor = fcolor2;\n"
    "}\n";

		GLint status;
		GLenum err;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);
    glCompileShader(vs);
		err = glGetError();
    if (err != GL_NO_ERROR) {
        PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
    }
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int loglen;
        char logbuffer[1000];
        glGetShaderInfoLog(vs, sizeof(logbuffer), &loglen, logbuffer);
        PRINTDB("OpenGL Program Compile Vertex Shader Error:\n%s", logbuffer);
		}

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);
    glCompileShader(fs);
		err = glGetError();
    if (err != GL_NO_ERROR) {
        PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
    }
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int loglen;
        char logbuffer[1000];
        glGetShaderInfoLog(fs, sizeof(logbuffer), &loglen, logbuffer);
        PRINTDB("OpenGL Program Compile Fragement Shader Error:\n%s", logbuffer);
		}

    GLuint vbo_shader_prog = glCreateProgram();
    glAttachShader(vbo_shader_prog, vs);
    glAttachShader(vbo_shader_prog, fs);
    glLinkProgram(vbo_shader_prog);

    vbo_shader_settings[0] = vbo_shader_prog;
    vbo_shader_settings[1] = glGetAttribLocation(vbo_shader_prog, "color1");
    vbo_shader_settings[2] = glGetAttribLocation(vbo_shader_prog, "color2");
    vbo_shader_settings[3] = glGetAttribLocation(vbo_shader_prog, "trig");
    vbo_shader_settings[4] = glGetAttribLocation(vbo_shader_prog, "pos_b");
    vbo_shader_settings[5] = glGetAttribLocation(vbo_shader_prog, "pos_c");
    vbo_shader_settings[6] = glGetAttribLocation(vbo_shader_prog, "mask");
    vbo_shader_settings[7] = glGetUniformLocation(vbo_shader_prog, "xscale");
    vbo_shader_settings[8] = glGetUniformLocation(vbo_shader_prog, "yscale");

    err = glGetError();
    if (err != GL_NO_ERROR) {
        PRINTDB("OpenGL Error: %s\n", gluErrorString(err));
    }

    glGetProgramiv(vbo_shader_prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int loglen;
        char logbuffer[1000];
        glGetProgramInfoLog(vbo_shader_prog, sizeof(logbuffer), &loglen, logbuffer);
        PRINTDB("OpenGL Program Linker Error:\n%s", logbuffer);
    } else {
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
    }
}

void VBORenderer::resize(int w, int h)
{
    vbo_shader_settings[9] = w;
    vbo_shader_settings[10] = h;
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

void VBORenderer::create_triangle(std::vector<Vertex> &vertices, const Color4f &color,
					const Vector3d &p0, const Vector3d &p1, const Vector3d &p2,
				  bool e0, bool e1, bool e2, bool mirrored) const
{
	double ax = p1[0] - p0[0], bx = p1[0] - p2[0];
	double ay = p1[1] - p0[1], by = p1[1] - p2[1];
	double az = p1[2] - p0[2], bz = p1[2] - p2[2];
	double nx = ay*bz - az*by;
	double ny = az*bx - ax*bz;
	double nz = ax*by - ay*bx;
	double nl = sqrt(nx*nx + ny*ny + nz*nz);

	GLbyte e0f = e0 ? 2 : -1;
	GLbyte e1f = e1 ? 2 : -1;
	GLbyte e2f = e2 ? 2 : -1;

	Vertex vertex = {
		{(float)p0[0], (float)p0[1], (float)p0[2]},
		{(float)(nx/nl), (float)(ny/nl), (float)(nz/nl)},
		{color[0], color[1], color[2], color[3]},
		{(color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0},
		{e0f, e1f, e2f},
		{(float)p1[0], (float)p1[1], (float)p1[2]},
		{(float)p2[0], (float)p2[1], (float)p2[2]},
		{0, 1, 0}
	};
	vertices.push_back(vertex);

	if (!mirrored) {
		vertex = {
			{(float)p1[0], (float)p1[1], (float)p1[2]},
			{(float)(nx/nl), (float)(ny/nl), (float)(nz/nl)},
			{color[0], color[1], color[2], color[3]},
			{(color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0},
			{e0f, e1f, e2f},
			{(float)p0[0], (float)p0[1], (float)p0[2]},
			{(float)p2[0], (float)p2[1], (float)p2[2]},
			{0, 0, 1}
		};
		vertices.push_back(vertex);
	}

	vertex = {
		{(float)p2[0], (float)p2[1], (float)p2[2]},
		{(float)(nx/nl), (float)(ny/nl), (float)(nz/nl)},
		{color[0], color[1], color[2], color[3]},
		{(color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0},
		{e0f, e1f, e2f},
		{(float)p0[0], (float)p0[1], (float)p0[2]},
		{(float)p1[0], (float)p1[1], (float)p1[2]},
		{1, 0, 0}
	};
	vertices.push_back(vertex);

	if (mirrored) {
		vertex = {
			{(float)p1[0], (float)p1[1], (float)p1[2]},
			{(float)(nx/nl), (float)(ny/nl), (float)(nz/nl)},
			{color[0], color[1], color[2], color[3]},
			{(color[0]+1)/2, (color[1]+1)/2, (color[2]+1)/2, 1.0},
			{e0f, e1f, e2f},
			{(float)p0[0], (float)p0[1], (float)p0[2]},
			{(float)p2[0], (float)p2[1], (float)p2[2]},
			{0, 0, 1}
		};
		vertices.push_back(vertex);
	}
}

void VBORenderer::create_surface(shared_ptr<const Geometry> geom, std::vector<Vertex> &render_buffer,
				 VertexSet &vertex_set, GLintptr prev_start_offset, GLsizei prev_draw_size,
				 csgmode_e csgmode, const Transform3d &m, const Color4f &color)
{
	shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom);

	size_t render_buffer_start_size = render_buffer.size();

	if (!ps) { return; }

	bool mirrored = m.matrix().determinant() < 0;

	if (ps->getDimension() == 2) {
		// Render 2D objects 1mm thick, but differences slightly larger
		double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);
		// Render top+bottom
		for (double z = -zbase/2; z < zbase; z += zbase) {
			fflush(stderr);
			for (size_t i = 0; i < ps->polygons.size(); i++) {
				const Polygon *poly = &ps->polygons[i];
				Vector3d p0 = poly->at(0); p0[2] += z; p0 = m * p0;
				Vector3d p1 = poly->at(1); p1[2] += z; p1 = m * p1;
				Vector3d p2 = poly->at(2); p2[2] += z; p2 = m * p2;

				if (poly->size() == 3) {
					if (z < 0) {
						create_triangle(render_buffer, color, p0, p2, p1, true, true, true, mirrored);
					} else {
						create_triangle(render_buffer, color, p0, p1, p2, true, true, true, mirrored);
					}
				}
				else if (poly->size() == 4) {
					Vector3d p3 = poly->at(3); p3[2] += z; p3 = m * p3;

					if (z < 0) {
						create_triangle(render_buffer, color, p0, p3, p1, true, false, true, mirrored);
						create_triangle(render_buffer, color, p2, p1, p3, true, false, true, mirrored);
					} else {
						create_triangle(render_buffer, color, p0, p1, p3, true, false, true, mirrored);
						create_triangle(render_buffer, color, p2, p3, p1, true, false, true, mirrored);
					}
				}
				else {
					Vector3d center = Vector3d::Zero();
					for (size_t j = 0; j < poly->size(); j++) {
						center[0] += poly->at(j)[0];
						center[1] += poly->at(j)[1];
					}
					center[0] /= poly->size();
					center[1] /= poly->size();

					for (size_t j = 1; j <= poly->size(); j++) {
						Vector3d p0 = center; center[2] += z; p0 = m * p0;
						Vector3d p1 = poly->at(j % poly->size()); p1[2] += z; p1 = m * p1;
						Vector3d p2 = poly->at(j - 1); p2[2] += z; p2 = m * p2;

						if (z < 0) {
							create_triangle(render_buffer, color, p0, p1, p2, false, true, false, mirrored);
						} else {
							create_triangle(render_buffer, color, p0, p2, p1, false, true, false, mirrored);
						}
					}
				}
			}
		}

		// Render sides
		if (ps->getPolygon().outlines().size() > 0) {
			for (const Outline2d &o : ps->getPolygon().outlines()) {
				for (size_t j = 1; j <= o.vertices.size(); j++) {
					Vector3d p1 = m * Vector3d(o.vertices[j-1][0], o.vertices[j-1][1], -zbase/2);
					Vector3d p2 = m * Vector3d(o.vertices[j-1][0], o.vertices[j-1][1], zbase/2);
					Vector3d p3 = m * Vector3d(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], -zbase/2);
					Vector3d p4 = m * Vector3d(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], zbase/2);

					create_triangle(render_buffer, color, p2, p1, p3,true, true, false, mirrored);
					create_triangle(render_buffer, color, p2, p3, p4, false, true, true, mirrored);
				}
			}
		}
		else {
			// If we don't have borders, use the polygons as borders.
			// FIXME: When is this used?
			const Polygons *borders_p = &ps->polygons;
			for (size_t i = 0; i < borders_p->size(); i++) {
				const Polygon *poly = &borders_p->at(i);
				for (size_t j = 1; j <= poly->size(); j++) {
					Vector3d p1 = poly->at(j - 1); p1[2] -= zbase/2; p1 = m * p1;
					Vector3d p2 = poly->at(j - 1); p2[2] += zbase/2; p2 = m * p2;
					Vector3d p3 = poly->at(j % poly->size()); p3[2] -= zbase/2; p3 = m * p3;
					Vector3d p4 = poly->at(j % poly->size()); p4[2] += zbase/2; p4 = m * p4;

					create_triangle(render_buffer, color, p2, p1, p3, true, true, false, mirrored);
					create_triangle(render_buffer, color, p2, p3, p4, false, true, true, mirrored);
				}
			}
		}

		vertex_set.draw_size = render_buffer.size() - render_buffer_start_size;
		vertex_set.start_offset = 0;
		if (render_buffer_start_size) {
			vertex_set.start_offset = prev_start_offset + prev_draw_size*sizeof(Vertex);
		}

	} else if (ps->getDimension() == 3) {
		for (size_t i = 0; i < ps->polygons.size(); i++) {
			const Polygon *poly = &ps->polygons[i];
			Vector3d p0 = m * poly->at(0);
			Vector3d p1 = m * poly->at(1);
			Vector3d p2 = m * poly->at(2);

			if (poly->size() == 3) {
				create_triangle(render_buffer, color, p0, p1, p2, true, true, true, mirrored);
			}
			else if (poly->size() == 4) {
				Vector3d p3 = m * poly->at(3);

				create_triangle(render_buffer, color, p0, p1, p3, true, false, true, mirrored);
				create_triangle(render_buffer, color, p2, p3, p1, true, false, true, mirrored);
			}
			else {
				Vector3d center = Vector3d::Zero();
				for (size_t j = 0; j < poly->size(); j++) {
					center[0] += poly->at(j)[0];
					center[1] += poly->at(j)[1];
					center[2] += poly->at(j)[2];
				}
				center[0] /= poly->size();
				center[1] /= poly->size();
				center[2] /= poly->size();
				for (size_t j = 1; j <= poly->size(); j++) {
					Vector3d p0 = m * center;
					Vector3d p1 = m * poly->at(j % poly->size());
					Vector3d p2 = m * poly->at(j - 1);

					create_triangle(render_buffer, color, p0, p2, p1, false, true, false, mirrored);
				}
			}
		}

    vertex_set.draw_size = render_buffer.size() - render_buffer_start_size;
    vertex_set.start_offset = 0;
    if (render_buffer_start_size) {
      vertex_set.start_offset = prev_start_offset + prev_draw_size*sizeof(Vertex);
    }
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
}

void VBORenderer::draw_surface(const VertexSet &vertex_set, bool use_color_array, bool use_edge_shader) const
{
	if (use_edge_shader) {
		glUniform1f(vbo_shader_settings[7], vbo_shader_settings[9]);
		glUniform1f(vbo_shader_settings[8], vbo_shader_settings[10]);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	if (use_color_array || use_edge_shader) {
		glEnableClientState(GL_NORMAL_ARRAY);
	}
	if (use_color_array) {
		glEnableClientState(GL_COLOR_ARRAY);
	}
	if (use_edge_shader) {
		glEnableVertexAttribArray(vbo_shader_settings[1]);
		glEnableVertexAttribArray(vbo_shader_settings[2]);
		glEnableVertexAttribArray(vbo_shader_settings[3]);
		glEnableVertexAttribArray(vbo_shader_settings[4]);
		glEnableVertexAttribArray(vbo_shader_settings[5]);
		glEnableVertexAttribArray(vbo_shader_settings[6]);
	}

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, position)));
	if (use_color_array || use_edge_shader) {
		glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, normal)));
	}
	if (use_color_array) {
		glColorPointer(4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, color1)));
	}

	if (use_edge_shader) {
		glVertexAttribPointer(vbo_shader_settings[1], 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, color1)));
		glVertexAttribPointer(vbo_shader_settings[2], 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, color2)));
		glVertexAttribPointer(vbo_shader_settings[3], 3, GL_BYTE, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, trig)));
		glVertexAttribPointer(vbo_shader_settings[4], 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, pos_b)));
		glVertexAttribPointer(vbo_shader_settings[5], 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, pos_c)));
		glVertexAttribPointer(vbo_shader_settings[6], 3, GL_BYTE, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(vertex_set.start_offset + offsetof(Vertex, mask)));
	}

	glDrawArrays(GL_TRIANGLES, 0, vertex_set.draw_size);

	if (use_edge_shader) {
		glDisableVertexAttribArray(vbo_shader_settings[1]);
		glDisableVertexAttribArray(vbo_shader_settings[2]);
		glDisableVertexAttribArray(vbo_shader_settings[3]);
		glDisableVertexAttribArray(vbo_shader_settings[4]);
		glDisableVertexAttribArray(vbo_shader_settings[5]);
		glDisableVertexAttribArray(vbo_shader_settings[6]);
	}

	if (use_color_array) {
		glDisableClientState(GL_COLOR_ARRAY);
	}
	if (use_color_array || use_edge_shader) {
		glDisableClientState(GL_NORMAL_ARRAY);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}
