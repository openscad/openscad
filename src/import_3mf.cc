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

#include "importnode.h"

#include "polyset.h"
#include "Geometry.h"
#include "printutils.h"

#ifdef ENABLE_LIB3MF
#include "NMR_DLLInterfaces.h"
#undef BOOL
using namespace NMR;

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

typedef std::list<std::shared_ptr<PolySet>> polysets_t;

static Geometry * import_3mf_error(PLib3MFModel *model = NULL, PLib3MFModelResourceIterator *object_it = NULL, PolySet *mesh = NULL)
{
	if (model) {
		lib3mf_release(model);
	}
	if (object_it) {
		lib3mf_release(object_it);
	}
	if (mesh) {
		delete mesh;
	}

	return new PolySet(3);
}

Geometry * import_3mf(const std::string &filename)
{
	DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
	HRESULT result = lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
	if (result != LIB3MF_OK) {
		PRINT("ERROR: Error reading 3MF library version");
		return new PolySet(3);
	}

	if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
		PRINTB("ERROR: Invalid 3MF library major version %d.%d.%d, expected %d.%d.%d",
                        interfaceVersionMajor % interfaceVersionMinor % interfaceVersionMicro %
                        NMR_APIVERSION_INTERFACE_MAJOR % NMR_APIVERSION_INTERFACE_MINOR % NMR_APIVERSION_INTERFACE_MICRO);
		return new PolySet(3);
	}

	PLib3MFModel *model;
	result = lib3mf_createmodel(&model);
	if (result != LIB3MF_OK) {
		PRINTB("ERROR: Could not create model: %08lx", result);
		return import_3mf_error();
	}

	PLib3MFModelReader *reader;
	result = lib3mf_model_queryreader(model, "3mf", &reader);
	if (result != LIB3MF_OK) {
		PRINTB("ERROR: Could not create 3MF reader: %08lx", result);
		return import_3mf_error(model);
	}

	result = lib3mf_reader_readfromfileutf8(reader, filename.c_str());
	lib3mf_release(reader);
	if (result != LIB3MF_OK) {
		PRINTB("WARNING: Could not read file '%s'", filename.c_str());
		return import_3mf_error(model);
	}

	PLib3MFModelResourceIterator *object_it;
	result = lib3mf_model_getmeshobjects(model, &object_it);
	if (result != LIB3MF_OK) {
		return import_3mf_error(model);
	}

	PolySet *first_mesh = 0;
	polysets_t meshes;
	unsigned int mesh_idx = 0;
	while (true) {
		int has_next;
		result = lib3mf_resourceiterator_movenext(object_it, &has_next);
		if (result != LIB3MF_OK) {
			return import_3mf_error(model, object_it, first_mesh);
		}
		if (!has_next) {
			break;
		}

		PLib3MFModelResource *object;
		result = lib3mf_resourceiterator_getcurrent(object_it, &object);
		if (result != LIB3MF_OK) {
			return import_3mf_error(model, object_it, first_mesh);
		}

		DWORD vertex_count;
		result = lib3mf_meshobject_getvertexcount(object, &vertex_count);
		if (result != LIB3MF_OK) {
			return import_3mf_error(model, object_it, first_mesh);
		}
		DWORD triangle_count;
		result = lib3mf_meshobject_gettrianglecount(object, &triangle_count);
		if (result != LIB3MF_OK) {
			return import_3mf_error(model, object_it, first_mesh);
		}

		PRINTDB("%s: mesh %d, vertex count: %lu, triangle count: %lu", filename.c_str() % mesh_idx % vertex_count % triangle_count);

		PolySet *p = new PolySet(3);
		for (DWORD idx = 0;idx < triangle_count;idx++) {
			MODELMESHTRIANGLE triangle;
			if (lib3mf_meshobject_gettriangle(object, idx, &triangle) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh);
			}
			
			MODELMESHVERTEX vertex1, vertex2, vertex3;
			if (lib3mf_meshobject_getvertex(object, triangle.m_nIndices[0], &vertex1) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh);
			}
			if (lib3mf_meshobject_getvertex(object, triangle.m_nIndices[1], &vertex2) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh);
			}
			if (lib3mf_meshobject_getvertex(object, triangle.m_nIndices[2], &vertex3) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh);
			}
			
			p->append_poly();
			p->append_vertex(vertex1.m_fPosition[0], vertex1.m_fPosition[1], vertex1.m_fPosition[2]);
			p->append_vertex(vertex2.m_fPosition[0], vertex2.m_fPosition[1], vertex2.m_fPosition[2]);
			p->append_vertex(vertex3.m_fPosition[0], vertex3.m_fPosition[1], vertex3.m_fPosition[2]);
		}

		if (first_mesh) {
			meshes.push_back(std::shared_ptr<PolySet>(p));
		} else {
			first_mesh = p;
		}
		mesh_idx++;
	}

	lib3mf_release(object_it);
	lib3mf_release(model);

	if (first_mesh == 0) {
		return new PolySet(3);
	} else if (meshes.empty()) {
		return first_mesh;
	} else {
		PolySet *p = new PolySet(3);
#ifdef ENABLE_CGAL
		Geometry::Geometries children;
		children.push_back(std::make_pair((const AbstractNode*)NULL,  shared_ptr<const Geometry>(first_mesh)));
		for (polysets_t::iterator it = meshes.begin();it != meshes.end();it++) {
			children.push_back(std::make_pair((const AbstractNode*)NULL,  shared_ptr<const Geometry>(*it)));
		}
		CGAL_Nef_polyhedron *N = CGALUtils::applyOperator(children, OpenSCADOperator::UNION);

		CGALUtils::createPolySetFromNefPolyhedron3(*N->p3, *p);
		delete N;
#endif
		return p;
	}
}

#else // ENABLE_LIB3MF

Geometry * import_3mf(const std::string &)
{
	PRINT("WARNING: Import from 3MF format was not enabled when building the application.");
	return new PolySet(3);
}

#endif // ENABLE_LIB3MF