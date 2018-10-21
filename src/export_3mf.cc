/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2016 Clifford Wolf <clifford@clifford.at> and
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

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "printutils.h"

#ifdef ENABLE_LIB3MF
#ifdef ENABLE_CGAL
#include "NMR_DLLInterfaces.h"
#undef BOOL
using namespace NMR;

#include <algorithm>

#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

bool triangle_sort_predicate (const CGAL_Triangle_3 t1, const CGAL_Triangle_3 t2) {
	if (t1.vertex(0) == t2.vertex(0)) {
		if (t1.vertex(1) == t2.vertex(1)) {
			return t1.vertex(2) < t2.vertex(2);
		}
		return t1.vertex(1) < t2.vertex(1);
	}
	return t1.vertex(0) < t2.vertex(0);
}

static uint32_t lib3mf_write_callback(const char *data, uint32_t bytes, std::ostream *stream)
{
	stream->write(data, bytes);
	return !(*stream);
}

static uint32_t lib3mf_seek_callback(uint64_t pos, std::ostream *stream)
{
	stream->seekp(pos);
	return !(*stream);
}

static void export_3mf_error(const std::string msg, PLib3MFModel *model = NULL)
{
	PRINT(msg);
	if (model) {
		lib3mf_release(model);
	}
}

/*!
    Saves the current 3D CGAL Nef polyhedron as 3MF to the given file.
    The file must be open.
 */
static void append_3mf(const CGAL_Nef_polyhedron &root_N, std::ostream &output)
{
	if (!root_N.p3 || !root_N.p3->is_simple()) {
		PRINT("WARNING: Export failed, the object isn't a valid 2-manifold.");
		return;
	}

	DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
	HRESULT result = lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
	if (result != LIB3MF_OK) {
		PRINT("ERROR: Error reading 3MF library version");
		return;
	}

	if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
		PRINTB("ERROR: Invalid 3MF library major version %d.%d.%d, expected %d.%d.%d",
                        interfaceVersionMajor % interfaceVersionMinor % interfaceVersionMicro %
                        NMR_APIVERSION_INTERFACE_MAJOR % NMR_APIVERSION_INTERFACE_MINOR % NMR_APIVERSION_INTERFACE_MICRO);
		return;
	}

	PLib3MFModel *model;
	result = lib3mf_createmodel(&model);
	if (result != LIB3MF_OK) {
		export_3mf_error("ERROR: Can't create 3MF model.");
		return;
	}

	PLib3MFModelMeshObject *mesh;
	if (lib3mf_model_addmeshobject(model, &mesh) != LIB3MF_OK) {
		export_3mf_error("ERROR: Can't add mesh to 3MF model.", model);
		return;
	}
	if (lib3mf_object_setnameutf8(mesh, "OpenSCAD Model") != LIB3MF_OK) {
		export_3mf_error("ERROR: Can't set name for 3MF model.", model);
		return;
	}

	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		root_N.p3->convert_to_Polyhedron(P);

		typedef CGAL_Polyhedron::Vertex Vertex;
		typedef CGAL_Polyhedron::Vertex_const_iterator VCI;
		typedef CGAL_Polyhedron::Facet_const_iterator FCI;
		typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

		// use sorted sets to get a stable sort order in the exported file
		typedef std::set<CGAL_Polyhedron::Point_3> vertex_set_t;
		typedef std::vector<CGAL_Triangle_3> triangle_list_t;

		vertex_set_t vertices;
		triangle_list_t triangles;

		for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
			HFCC hc = fi->facet_begin();
			HFCC hc_end = hc;
			Vertex v1, v2, v3;
			v1 = *VCI((hc++)->vertex());
			vertices.insert(v1.point());
			v3 = *VCI((hc++)->vertex());
			vertices.insert(v3.point());
			do {
				v2 = v3;
				v3 = *VCI((hc++)->vertex());

				CGAL_Polyhedron::Point_3 p1, p2, p3;
				p1 = v1.point();
				p2 = v2.point();
				p3 = v3.point();
				vertices.insert(p3);

				triangles.push_back(CGAL_Triangle_3(p1, p2, p3));
			} while (hc != hc_end);
		}

		for (const auto &vertex : vertices) {
			MODELMESHVERTEX v;
			v.m_fPosition[0] = CGAL::to_double(vertex.x());
			v.m_fPosition[1] = CGAL::to_double(vertex.y());
			v.m_fPosition[2] = CGAL::to_double(vertex.z());
			if (lib3mf_meshobject_addvertex(mesh, &v, NULL) != LIB3MF_OK) {
				export_3mf_error("ERROR: Can't add vertex to 3MF model.", model);
				return;
			}
		}

		std::sort(triangles.begin(), triangles.end(), triangle_sort_predicate);
		for (const auto &triangle : triangles) {
			MODELMESHTRIANGLE t;
			t.m_nIndices[0] = std::distance(vertices.begin(), std::find(vertices.begin(), vertices.end(), triangle.vertex(0)));
			t.m_nIndices[1] = std::distance(vertices.begin(), std::find(vertices.begin(), vertices.end(), triangle.vertex(1)));
			t.m_nIndices[2] = std::distance(vertices.begin(), std::find(vertices.begin(), vertices.end(), triangle.vertex(2)));
			if (lib3mf_meshobject_addtriangle(mesh, &t, NULL) != LIB3MF_OK) {
				export_3mf_error("ERROR: Can't add triangle to 3MF model.", model);
				return;
			}
		}

		PLib3MFModelBuildItem *builditem;
		if (lib3mf_model_addbuilditem(model, mesh, NULL, &builditem) != LIB3MF_OK) {
			export_3mf_error("ERROR: Can't add triangle to 3MF model.", model);
			return;
		}

		PLib3MFModelWriter *writer;
		if (lib3mf_model_querywriter(model, "3mf", &writer) != LIB3MF_OK) {
			export_3mf_error("ERROR: Can't get writer for 3MF model.", model);
			return;
		}

		result = lib3mf_writer_writetocallback(writer, (void *)lib3mf_write_callback, (void *)lib3mf_seek_callback, &output);
		output.flush();
		lib3mf_release(writer);
		lib3mf_release(model);
		if (result != LIB3MF_OK) {
			export_3mf_error("ERROR: Error writing 3MF model.");
		}
	} catch (CGAL::Assertion_exception& e) {
		PRINTB("ERROR: CGAL error in CGAL_Nef_polyhedron3::convert_to_Polyhedron(): %s", e.what());
	}
	CGAL::set_error_behaviour(old_behaviour);
}

static void append_3mf(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(geom.get())) {
		append_3mf(*N, output);
	}
	else if (const PolySet *ps = dynamic_cast<const PolySet *>(geom.get())) {
		// FIXME: Implement this without creating a Nef polyhedron
		CGAL_Nef_polyhedron *N = CGALUtils::createNefPolyhedronFromGeometry(*ps);
		append_3mf(*N, output);
		delete N;
	}
	else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}
}

void export_3mf(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	append_3mf(geom, output);
}

#endif // ENABLE_CGAL

#else // ENABLE_LIB3MF

void export_3mf(const shared_ptr<const Geometry> &, std::ostream &)
{
	PRINT("Export to 3MF format was not enabled when building the application.");
}

#endif // ENABLE_LIB3MF