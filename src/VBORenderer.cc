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
}

void VBORenderer::resize(int w, int h)
{
	Renderer::resize(w,h);
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
	if (points.size() == 3 && getShader().data.csg_rendering.barycentric) {
		// Get edge states
		std::array<GLshort, 3> barycentric_flags;

		if (!outlines) {
			// top / bottom or 3d object
			if (shape_size == 3) {
				//true, true, true
				barycentric_flags = {0, 0, 0};
			} else if (shape_size == 4) {
				//false, true, true
				barycentric_flags = {1, 0, 0};
			} else {
				//true, false, false
				barycentric_flags = {0, 1, 1};
			}
		} else {
			// sides
			if (primitive_index == 0) {
				//true, false, true
				barycentric_flags = {0, 1, 0};
			} else {
				//true, true, false
				barycentric_flags = {0, 0, 1};
			}
		}
		
		barycentric_flags[active_point_index] = 1;

		addAttributeValues(*(vertex_data->attributes()[BARYCENTRIC_ATTRIB]), barycentric_flags[0], barycentric_flags[1], barycentric_flags[2]);

		if (OpenSCAD::debug != "") PRINTDB("create_vertex barycentric : [%d, %d, %d]",
			barycentric_flags[0] % barycentric_flags[1] % barycentric_flags[2]);

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

void VBORenderer::create_surface(const PolySet &ps, VertexArray &vertex_array,
				 csgmode_e csgmode, const Transform3d &m, const Color4f &color) const
{
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!vertex_data) { return; }

	bool mirrored = m.matrix().determinant() < 0;
	size_t triangle_count = 0;

	if (ps.getDimension() == 2) {
		create_polygons(ps, vertex_array, csgmode, m, color);
	} else if (ps.getDimension() == 3) {
		VertexStates &vertex_states = vertex_array.states();
		size_t last_size = vertex_data->sizeInBytes();

		for (const auto &poly : ps.polygons) {
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

void VBORenderer::create_edges(const PolySet &ps,
				VertexArray &vertex_array, csgmode_e csgmode,
				const Transform3d &m,
				const Color4f &color) const
{
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!vertex_data) return;

	VertexStates &vertex_states = vertex_array.states();

	if (ps.getDimension() == 2) {
		if (csgmode == Renderer::CSGMODE_NONE) {
			// Render only outlines
			for (const Outline2d &o : ps.getPolygon().outlines()) {
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
			for (const Outline2d &o : ps.getPolygon().outlines()) {
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
	} else if (ps.getDimension() == 3) {
		for (const auto &polygon : ps.polygons) {
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

void VBORenderer::create_polygons(const PolySet &ps, VertexArray &vertex_array,
				  csgmode_e csgmode, const Transform3d &m, const Color4f &color) const
{
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!vertex_data) return;

	VertexStates &vertex_states = vertex_array.states();

	if (ps.getDimension() == 2) {
		PRINTD("create_polygons 2D");
		bool mirrored = m.matrix().determinant() < 0;
		size_t triangle_count = 0;
		size_t last_size = vertex_data->sizeInBytes();

		if (csgmode == Renderer::CSGMODE_NONE) {
			PRINTD("create_polygons CSGMODE_NONE");
			for (const auto &poly : ps.polygons) {
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
				for (const auto &poly : ps.polygons) {
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
			if (ps.getPolygon().outlines().size() > 0) {
				PRINTD("Render outlines as sides");
				for (const Outline2d &o : ps.getPolygon().outlines()) {
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
				for (const auto &poly : ps.polygons) {
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
	shader_data->addAttributeData(std::make_shared<AttributeData<GLshort,3,GL_SHORT>>()); // barycentric
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

	if (getShader().data.csg_rendering.barycentric) {
		index = getShader().data.csg_rendering.barycentric;
		count = shader_data->attributes()[BARYCENTRIC_ATTRIB]->count();
		type = shader_data->attributes()[BARYCENTRIC_ATTRIB]->glType();
		stride = shader_data->stride();
		offset = shader_start_offset + shader_data->interleavedOffset(BARYCENTRIC_ATTRIB);
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
	if (OpenSCAD::debug != "") PRINTDB("glEnableVertexAttribArray(%d)", getShader().data.csg_rendering.barycentric);
	glEnableVertexAttribArray(getShader().data.csg_rendering.barycentric);
}
void VBORenderer::shader_attribs_disable() const
{
	if (OpenSCAD::debug != "") PRINTDB("glDisableVertexAttribArray(%d)", getShader().data.csg_rendering.barycentric);
	glDisableVertexAttribArray(getShader().data.csg_rendering.barycentric);
}