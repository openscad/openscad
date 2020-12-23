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
#include "polyset-utils.h"
#include "csgnode.h"
#include "printutils.h"
#include "hash.h"

#include <cstddef>
#include <iomanip>
#include <sstream>

VBORenderer::VBORenderer()
	: Renderer(), shader_attributes_index(0)
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

size_t VBORenderer::getSurfaceBufferSize(const std::shared_ptr<CSGProducts> &products, bool highlight_mode, bool background_mode, bool unique_geometry) const
{
	size_t buffer_size = 0;
	if (unique_geometry) this->geomVisitMark.clear();

	for (const auto &product : products->products) {
		for (const auto &csgobj : product.intersections) {
			buffer_size += getSurfaceBufferSize(csgobj, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION);
		}
		for (const auto &csgobj : product.subtractions) {
			buffer_size += getSurfaceBufferSize(csgobj, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE);
		}
	}
	return buffer_size;
}

size_t VBORenderer::getSurfaceBufferSize(const CSGChainObject &csgobj, bool highlight_mode, bool background_mode, const OpenSCADOperator type, bool unique_geometry) const
{
	size_t buffer_size = 0;
	if (unique_geometry && this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return 0;
	csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);

	if (csgobj.leaf->geom) {
		const PolySet* ps = dynamic_cast<const PolySet*>(csgobj.leaf->geom.get());
		if (ps) {
			buffer_size += getSurfaceBufferSize(*ps, csgmode);
		}
	}
	return buffer_size;
}

size_t VBORenderer::getSurfaceBufferSize(const PolySet &polyset, csgmode_e csgmode) const
{
	size_t buffer_size = 0;
	for (const auto &poly : polyset.polygons) {
		if (poly.size() == 3) {
			buffer_size++;
		} else if (poly.size() == 4) {
			buffer_size+=2;
		} else {
			buffer_size+=poly.size();
		}
	}						
	if (polyset.getDimension() == 2) {
		if (csgmode != CSGMODE_NONE) {
			buffer_size*=2; // top and bottom
			// sides
			if (polyset.getPolygon().outlines().size() > 0) {
				for (const Outline2d &o : polyset.getPolygon().outlines()) {
					buffer_size+=o.vertices.size()*2;
				}
			} else {
				for (const auto &poly : polyset.polygons) {
					buffer_size+=poly.size()*2;
				}
			}
		}
	}
	return buffer_size*3;
}

size_t VBORenderer::getEdgeBufferSize(const std::shared_ptr<CSGProducts> &products, bool highlight_mode, bool background_mode, bool unique_geometry) const
{
	size_t buffer_size = 0;
	if (unique_geometry) this->geomVisitMark.clear();

	for (const auto &product : products->products) {
		for (const auto &csgobj : product.intersections) {
			buffer_size += getEdgeBufferSize(csgobj, highlight_mode, background_mode, OpenSCADOperator::INTERSECTION, unique_geometry);
		}
		for (const auto &csgobj : product.subtractions) {
			buffer_size += getEdgeBufferSize(csgobj, highlight_mode, background_mode, OpenSCADOperator::DIFFERENCE, unique_geometry);
		}
	}
	return buffer_size;
}

size_t VBORenderer::getEdgeBufferSize(const CSGChainObject &csgobj, bool highlight_mode, bool background_mode, const OpenSCADOperator type, bool unique_geometry) const
{
	size_t buffer_size = 0;
	if (unique_geometry && this->geomVisitMark[std::make_pair(csgobj.leaf->geom.get(), &csgobj.leaf->matrix)]++ > 0) return 0;
	csgmode_e csgmode = get_csgmode(highlight_mode, background_mode, type);

	if (csgobj.leaf->geom) {
		const PolySet* ps = dynamic_cast<const PolySet*>(csgobj.leaf->geom.get());
		if (ps) {
			buffer_size += getEdgeBufferSize(*ps, csgmode);
		}
	}
	return buffer_size;
}

size_t VBORenderer::getEdgeBufferSize(const PolySet &polyset, csgmode_e csgmode) const
{
	size_t buffer_size = 0;
	if (polyset.getDimension() == 2) {
		// Render only outlines
		for (const Outline2d &o : polyset.getPolygon().outlines()) {
			buffer_size += o.vertices.size();
			if (csgmode != CSGMODE_NONE) {
				buffer_size += o.vertices.size();
				// Render sides
				buffer_size += o.vertices.size()*2;
			}
		}
	} else if (polyset.getDimension() == 3) {
		for (const auto &polygon : polyset.polygons) {
			buffer_size += polygon.size();
		}
	}
	return buffer_size;
}

void VBORenderer::add_shader_attributes(VertexArray &vertex_array,
					const std::array<Vector3d,3> &points,
					const std::array<Vector3d,3> &normals,
					const Color4f &color,
					size_t active_point_index, size_t primitive_index,
					double z_offset, size_t shape_size,
					size_t shape_dimensions, bool outlines,
					bool mirror) const
{
	if (!shader_attributes_index) return;
	
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (points.size() == 3 && getShader().data.csg_rendering.barycentric) {
		// Get edge states
		std::array<GLubyte, 3> barycentric_flags;

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

		addAttributeValues(*(vertex_data->attributes()[shader_attributes_index + BARYCENTRIC_ATTRIB]), barycentric_flags[0], barycentric_flags[1], barycentric_flags[2], 0);
	} else {
		if (OpenSCAD::debug != "") PRINTDB("add_shader_attributes bad points size = %d", points.size());
	}
}

void VBORenderer::create_vertex(VertexArray &vertex_array, const Color4f &color,
				const std::array<Vector3d,3> &points,
				const std::array<Vector3d,3> &normals,
				size_t active_point_index, size_t primitive_index,
				double z_offset, size_t shape_size,
				size_t shape_dimensions, bool outlines,
				bool mirror) const
{
	vertex_array.createVertex(points, normals, color, active_point_index,
				primitive_index, z_offset, shape_size,
				shape_dimensions, outlines, mirror,
				[this](VertexArray &vertex_array,
					const std::array<Vector3d,3> &points,
					const std::array<Vector3d,3> &normals,
					const Color4f &color,
					size_t active_point_index, size_t primitive_index,
					double z_offset, size_t shape_size,
					size_t shape_dimensions, bool outlines,
					bool mirror) -> void {
					this->add_shader_attributes(vertex_array, points, normals, color,
									active_point_index, primitive_index,
									z_offset, shape_size, shape_dimensions,
									outlines, mirror);
				}
			);

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
	Vector3d n = Vector3d(nx/nl, ny/nl, nz/nl);
	
	if (!vertex_array.data()) return;

	create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
			0, primitive_index, z_offset, shape_size,
			shape_dimensions, outlines, mirror);
	if (!mirror) {
		create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
				1, primitive_index, z_offset, shape_size,
				shape_dimensions, outlines, mirror);
	}
	create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
			2, primitive_index, z_offset, shape_size,
			shape_dimensions, outlines, mirror);
	if (mirror) {
		create_vertex(vertex_array, color, {p0, p1, p2}, {n, n, n},
				1, primitive_index, z_offset, shape_size,
				shape_dimensions, outlines, mirror);
	}
}

static Vector3d uniqueMultiply(std::unordered_map<Vector3d,size_t> &vert_mult_map,
				std::vector<Vector3d> &mult_verts, const Vector3d &in_vert,
				const Transform3d &m)
{
	Vector3d out_vert;
	size_t size = vert_mult_map.size();
	std::unordered_map<Vector3d,size_t>::iterator entry;
	entry = vert_mult_map.find(in_vert);
	if (entry == vert_mult_map.end()) {
		out_vert = m * in_vert;
		vert_mult_map.emplace(in_vert, size);
		mult_verts.emplace_back(out_vert);
	} else {
		out_vert = mult_verts[entry->second];
	}
	return out_vert;
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
		std::unordered_map<Vector3d,size_t> vert_mult_map;
		std::vector<Vector3d> mult_verts;
		size_t last_size = vertex_array.verticesOffset();
		
		size_t elements_offset = 0;
		if (vertex_array.useElements()) {
			elements_offset = vertex_array.elementsOffset();
			vertex_array.elementsMap().clear();
		}

		for (const auto &poly : ps.polygons) {
			if (poly.size() == 3) {
				Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(0), m);
				Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(1), m);
				Vector3d p2 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(2), m);

				create_triangle(vertex_array, color, p0, p1, p2,
					0, 0, poly.size(), 3, false, mirrored);
				triangle_count++;
			}
			else if (poly.size() == 4) {
				Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(0), m);
				Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(1), m);
				Vector3d p2 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(2), m);
				Vector3d p3 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(3), m);

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
					Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, center, m);
					Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(i % poly.size()), m);
					Vector3d p2 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(i - 1), m);
					
					create_triangle(vertex_array, color, p0, p2, p1,
						i-1, 0, poly.size(), 3, false, mirrored);
					triangle_count++;
				}
			}
		}

		GLenum elements_type = 0;
		if (vertex_array.useElements())
			elements_type = vertex_array.elementsData()->glType();
		std::shared_ptr<VertexState> vs = vertex_array.createVertexState(
			GL_TRIANGLES, triangle_count*3, elements_type,
			vertex_array.writeIndex(), elements_offset);
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
	std::unordered_map<Vector3d,size_t> vert_mult_map;
	std::vector<Vector3d> mult_verts;

	if (ps.getDimension() == 2) {
		if (csgmode == Renderer::CSGMODE_NONE) {
			// Render only outlines
			for (const Outline2d &o : ps.getPolygon().outlines()) {
				size_t last_size = vertex_array.verticesOffset();
				size_t elements_offset = 0;
				if (vertex_array.useElements()) {
					elements_offset = vertex_array.elementsOffset();
					vertex_array.elementsMap().clear();
				}
				for (const Vector2d &v : o.vertices) {
					Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, Vector3d(v[0],v[1],0.0), m);

					create_vertex(vertex_array, color, {p0}, {}, 0, 0, 0.0, o.vertices.size(), 2, true, false);
				}

				GLenum elements_type = 0;
				if (vertex_array.useElements())
					elements_type = vertex_array.elementsData()->glType();
				std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(
					GL_LINE_LOOP, o.vertices.size(), elements_type,
					vertex_array.writeIndex(), elements_offset);
				vertex_states.emplace_back(std::move(line_loop));
				vertex_array.addAttributePointers(last_size);
			}
		} else {
			// Render 2D objects 1mm thick, but differences slightly larger
			double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0.0);
			for (const Outline2d &o : ps.getPolygon().outlines()) {
				size_t last_size = vertex_array.verticesOffset();
				size_t elements_offset = 0;
				if (vertex_array.useElements()) {
					elements_offset = vertex_array.elementsOffset();
					vertex_array.elementsMap().clear();
				}

				// Render top+bottom outlines
				for (double z = -zbase/2; z < zbase; z += zbase) {
					for (const Vector2d &v : o.vertices) {
						Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, Vector3d(v[0],v[1],z), m);

						create_vertex(vertex_array, color, {p0}, {}, 0, 0, 0.0, o.vertices.size()*2, 2, true, false);
					}
				}

				GLenum elements_type = 0;
				if (vertex_array.useElements())
					elements_type = vertex_array.elementsData()->glType();
				std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(
					GL_LINE_LOOP, o.vertices.size()*2, elements_type,
					vertex_array.writeIndex(), elements_offset);
				vertex_states.emplace_back(std::move(line_loop));
				vertex_array.addAttributePointers(last_size);

				last_size = vertex_array.verticesOffset();
				if (vertex_array.useElements()) {
					elements_offset = vertex_array.elementsOffset();
					vertex_array.elementsMap().clear();
				}
				// Render sides
				for (const Vector2d &v : o.vertices) {
					Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, Vector3d(v[0], v[1], -zbase/2), m);
					Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, Vector3d(v[0], v[1], +zbase/2), m);

					create_vertex(vertex_array, color, {p0,p1}, {}, 0, 0, 0.0, o.vertices.size(), 2, true, false);
					create_vertex(vertex_array, color, {p0,p1}, {}, 1, 0, 0.0, o.vertices.size(), 2, true, false);
				}
				
				elements_type = 0;
				if (vertex_array.useElements())
					elements_type = vertex_array.elementsData()->glType();
				std::shared_ptr<VertexState> lines = vertex_array.createVertexState(
					GL_LINES, o.vertices.size()*2, elements_type,
					vertex_array.writeIndex(), elements_offset);
				vertex_states.emplace_back(std::move(lines));
				vertex_array.addAttributePointers(last_size);
			}
		}
	} else if (ps.getDimension() == 3) {
		for (const auto &polygon : ps.polygons) {
			size_t last_size = vertex_array.verticesOffset();
			size_t elements_offset = 0;
			if (vertex_array.useElements()) {
				elements_offset = vertex_array.elementsOffset();
				vertex_array.elementsMap().clear();
			}
			for (const auto &vertex : polygon) {
				Vector3d p = uniqueMultiply(vert_mult_map, mult_verts, vertex, m);
				
				create_vertex(vertex_array, color, {p}, {}, 0, 0, 0.0, polygon.size(), 2, true, false);
			}

			GLenum elements_type = 0;
			if (vertex_array.useElements())
				elements_type = vertex_array.elementsData()->glType();
			std::shared_ptr<VertexState> line_loop = vertex_array.createVertexState(
				GL_LINE_LOOP, polygon.size(), elements_type,
				vertex_array.writeIndex(), elements_offset);
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
	std::unordered_map<Vector3d,size_t> vert_mult_map;
	std::vector<Vector3d> mult_verts;

	if (ps.getDimension() == 2) {
		PRINTD("create_polygons 2D");
		bool mirrored = m.matrix().determinant() < 0;
		size_t triangle_count = 0;
		size_t last_size = vertex_array.verticesOffset();
		size_t elements_offset = 0;
		if (vertex_array.useElements()) {
			elements_offset = vertex_array.elementsOffset();
			vertex_array.elementsMap().clear();
		}

		if (csgmode == Renderer::CSGMODE_NONE) {
			PRINTD("create_polygons CSGMODE_NONE");
			for (const auto &poly : ps.polygons) {
				if (poly.size() == 3) {
					Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(0), m);
					Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(1), m);
					Vector3d p2 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(2), m);

					create_triangle(vertex_array, color, p0, p1, p2,
						0, 0, poly.size(), 2, false, mirrored);
					triangle_count++;
				}
				else if (poly.size() == 4) {
					Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(0), m);
					Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(1), m);
					Vector3d p2 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(2), m);
					Vector3d p3 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(3), m);

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
						Vector3d p0 = uniqueMultiply(vert_mult_map, mult_verts, center, m);
						Vector3d p1 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(i % poly.size()), m);
						Vector3d p2 = uniqueMultiply(vert_mult_map, mult_verts, poly.at(i - 1), m);

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
					if (poly.size() == 3) {
						Vector3d p0 = poly.at(0); p0[2] += z;
						Vector3d p1 = poly.at(1); p1[2] += z;
						Vector3d p2 = poly.at(2); p2[2] += z;
						
						p0 = uniqueMultiply(vert_mult_map, mult_verts, p0, m);
						p1 = uniqueMultiply(vert_mult_map, mult_verts, p1, m);
						p2 = uniqueMultiply(vert_mult_map, mult_verts, p2, m);
						
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
						Vector3d p0 = poly.at(0); p0[2] += z;
						Vector3d p1 = poly.at(1); p1[2] += z;
						Vector3d p2 = poly.at(2); p2[2] += z;
						Vector3d p3 = poly.at(3); p3[2] += z;
						
						p0 = uniqueMultiply(vert_mult_map, mult_verts, p0, m);
						p1 = uniqueMultiply(vert_mult_map, mult_verts, p1, m);
						p2 = uniqueMultiply(vert_mult_map, mult_verts, p2, m);
						p3 = uniqueMultiply(vert_mult_map, mult_verts, p3, m);
						
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
							Vector3d p0 = center; p0[2] += z; 
							Vector3d p1 = poly.at(i % poly.size()); p1[2] += z;
							Vector3d p2 = poly.at(i - 1); p2[2] += z;

							p0 = uniqueMultiply(vert_mult_map, mult_verts, p0, m);
							p1 = uniqueMultiply(vert_mult_map, mult_verts, p1, m);
							p2 = uniqueMultiply(vert_mult_map, mult_verts, p2, m);
							
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
						Vector3d p1 = Vector3d(o.vertices[i-1][0], o.vertices[i-1][1], -zbase/2);
						Vector3d p2 = Vector3d(o.vertices[i-1][0], o.vertices[i-1][1], zbase/2);
						Vector3d p3 = Vector3d(o.vertices[i % o.vertices.size()][0], o.vertices[i % o.vertices.size()][1], -zbase/2);
						Vector3d p4 = Vector3d(o.vertices[i % o.vertices.size()][0], o.vertices[i % o.vertices.size()][1], zbase/2);

						p1 = uniqueMultiply(vert_mult_map, mult_verts, p1, m);
						p2 = uniqueMultiply(vert_mult_map, mult_verts, p2, m);
						p3 = uniqueMultiply(vert_mult_map, mult_verts, p3, m);
						p4 = uniqueMultiply(vert_mult_map, mult_verts, p4, m);
						
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
						Vector3d p1 = poly.at(i - 1); p1[2] -= zbase/2;
						Vector3d p2 = poly.at(i - 1); p2[2] += zbase/2;
						Vector3d p3 = poly.at(i % poly.size()); p3[2] -= zbase/2;
						Vector3d p4 = poly.at(i % poly.size()); p4[2] += zbase/2;
						
						p1 = uniqueMultiply(vert_mult_map, mult_verts, p1, m);
						p2 = uniqueMultiply(vert_mult_map, mult_verts, p2, m);
						p3 = uniqueMultiply(vert_mult_map, mult_verts, p3, m);
						p4 = uniqueMultiply(vert_mult_map, mult_verts, p4, m);

						create_triangle(vertex_array, color, p2, p1, p3,
							0, 0, poly.size(), 2, true, mirrored);
						create_triangle(vertex_array, color, p2, p3, p4,
							1, 0, poly.size(), 2, true, mirrored);
						triangle_count+=2;
					}
				}
			}
		}
		
		GLenum elements_type = 0;
		if (vertex_array.useElements())
			elements_type = vertex_array.elementsData()->glType();
		std::shared_ptr<VertexState> vs = vertex_array.createVertexState(
			GL_TRIANGLES, triangle_count*3, elements_type,
			vertex_array.writeIndex(), elements_offset);
		vertex_states.emplace_back(std::move(vs));
		vertex_array.addAttributePointers(last_size);
	} else {
		assert(false && "Cannot render object with no dimension");
	}
}

void VBORenderer::add_shader_data(VertexArray &vertex_array)
{
	std::shared_ptr<VertexData> vertex_data = vertex_array.data();
	shader_attributes_index = vertex_data->attributes().size();
	vertex_data->addAttributeData(std::make_shared<AttributeData<GLubyte,4,GL_UNSIGNED_BYTE>>()); // barycentric
}

void VBORenderer::add_shader_pointers(VertexArray &vertex_array)
{
	shared_ptr<VertexData> vertex_data = vertex_array.data();

	if (!vertex_data) return;
	
	size_t start_offset = vertex_array.verticesOffset();

	std::shared_ptr<VertexState> ss = std::make_shared<VBOShaderVertexState>(vertex_array.writeIndex(), 0,
										 vertex_array.verticesVBO(),
										 vertex_array.elementsVBO());
	GLuint index = 0;
	GLsizei count = 0, stride = 0;
	GLenum type = 0;
	size_t offset = 0;

	if (getShader().data.csg_rendering.barycentric) {
		index = getShader().data.csg_rendering.barycentric;
		count = vertex_data->attributes()[shader_attributes_index+BARYCENTRIC_ATTRIB]->count();
		type = vertex_data->attributes()[shader_attributes_index+BARYCENTRIC_ATTRIB]->glType();
		stride = vertex_data->stride();
		offset = start_offset + vertex_data->interleavedOffset(shader_attributes_index+BARYCENTRIC_ATTRIB);
		ss->glBegin().emplace_back([index, count, type, stride, offset, ss_ptr = std::weak_ptr<VertexState>(ss)]() {
			auto ss = ss_ptr.lock();
			if (ss) {
				GL_TRACE("glVertexAttribPointer(%d, %d, %d, %p)", count % type % stride % (GLvoid *)(ss->drawOffset() + offset));
				glVertexAttribPointer(index, count, type, GL_FALSE, stride, (GLvoid *)(ss->drawOffset() + offset));
				GL_ERROR_CHECK();
			}
		});
	}

	vertex_array.states().emplace_back(std::move(ss));
}

void VBORenderer::shader_attribs_enable() const
{
	GL_TRACE("glEnableVertexAttribArray(%d)", getShader().data.csg_rendering.barycentric);
	glEnableVertexAttribArray(getShader().data.csg_rendering.barycentric); GL_ERROR_CHECK();
}
void VBORenderer::shader_attribs_disable() const
{
	GL_TRACE("glDisableVertexAttribArray(%d)", getShader().data.csg_rendering.barycentric);
	glDisableVertexAttribArray(getShader().data.csg_rendering.barycentric); GL_ERROR_CHECK();
}