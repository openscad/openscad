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
#include "version_helper.h"
#include "AST.h"
#include "boost-utils.h"

#ifdef ENABLE_LIB3MF
#ifndef LIB3MF_API_2
#include <Model/COM/NMR_DLLInterfaces.h>
#undef BOOL
using namespace NMR;

/*
 * Provided here for reference in LibraryInfo.cc which can't include
 * both Qt and lib3mf headers due to some conflicting definitions of
 * windows types when compiling with MinGW.
 */
const std::string get_lib3mf_version() {
	DWORD major, minor, micro;
	NMR::lib3mf_getinterfaceversion(&major, &minor, &micro);

	const OpenSCAD::library_version_number header_version{NMR_APIVERSION_INTERFACE_MAJOR, NMR_APIVERSION_INTERFACE_MINOR, NMR_APIVERSION_INTERFACE_MICRO};
	const OpenSCAD::library_version_number runtime_version{major, minor, micro};
	return OpenSCAD::get_version_string(header_version, runtime_version);
}

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

typedef std::list<std::shared_ptr<PolySet>> polysets_t;

static Geometry * import_3mf_error(PLib3MFModel *model = nullptr, PLib3MFModelResourceIterator *object_it = nullptr, PolySet *mesh = nullptr, PolySet *mesh2 = nullptr)
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
	if (mesh2) {
		delete mesh2;
	}

	return new PolySet(3);
}

Geometry * import_3mf(const std::string &filename, const Location &loc)
{
	DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
	HRESULT result = lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
	if (result != LIB3MF_OK) {
		LOG(message_group::Error,Location::NONE,"","Error reading 3MF library version");
		return new PolySet(3);
	}

	if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
		LOG(message_group::Error,Location::NONE,"","Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
                        interfaceVersionMajor,interfaceVersionMinor,interfaceVersionMicro,
                        NMR_APIVERSION_INTERFACE_MAJOR,NMR_APIVERSION_INTERFACE_MINOR,NMR_APIVERSION_INTERFACE_MICRO);
		return new PolySet(3);
	}

	PLib3MFModel *model;
	result = lib3mf_createmodel(&model);
	if (result != LIB3MF_OK) {
		LOG(message_group::Error,Location::NONE,"","Could not create model: %1$08lx",result);
		return import_3mf_error();
	}

	PLib3MFModelReader *reader;
	result = lib3mf_model_queryreader(model, "3mf", &reader);
	if (result != LIB3MF_OK) {
		LOG(message_group::Error,Location::NONE,"","Could not create 3MF reader: %1$08lx",result);
		return import_3mf_error(model);
	}

	result = lib3mf_reader_readfromfileutf8(reader, filename.c_str());
	lib3mf_release(reader);
	if (result != LIB3MF_OK) {
		LOG(message_group::Warning,Location::NONE,"","Could not read file '%1$s', import() at line %2$d",filename.c_str(),loc.firstLine());
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
		for (DWORD idx = 0; idx < triangle_count; ++idx) {
			MODELMESHTRIANGLE triangle;
			if (lib3mf_meshobject_gettriangle(object, idx, &triangle) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh, p);
			}
			
			MODELMESHVERTEX vertex1, vertex2, vertex3;
			if (lib3mf_meshobject_getvertex(object, triangle.m_nIndices[0], &vertex1) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh, p);
			}
			if (lib3mf_meshobject_getvertex(object, triangle.m_nIndices[1], &vertex2) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh, p);
			}
			if (lib3mf_meshobject_getvertex(object, triangle.m_nIndices[2], &vertex3) != LIB3MF_OK) {
				return import_3mf_error(model, object_it, first_mesh, p);
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
		for (polysets_t::iterator it = meshes.begin(); it != meshes.end(); ++it) {
			children.push_back(std::make_pair((const AbstractNode*)NULL,  shared_ptr<const Geometry>(*it)));
		}
		CGAL_Nef_polyhedron *N = CGALUtils::applyUnion3D(children.begin(), children.end());

		CGALUtils::createPolySetFromNefPolyhedron3(*N->p3, *p);
		delete N;
#endif
		return p;
	}
}

#else // LIB3MF_API_2

#include "lib3mf_implicit.hpp"

/*
 * Provided here for reference in LibraryInfo.cc which can't include
 * both Qt and lib3mf headers due to some conflicting definitions of
 * windows types when compiling with MinGW.
 */
const std::string get_lib3mf_version() {
	Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
	Lib3MF::PWrapper wrapper;
	
	try {
		wrapper = Lib3MF::CWrapper::loadLibrary();
		wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
	}

	const OpenSCAD::library_version_number header_version{LIB3MF_VERSION_MAJOR, LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO};
	const OpenSCAD::library_version_number runtime_version{interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro};
	return OpenSCAD::get_version_string(header_version, runtime_version);
}

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

typedef std::list<std::shared_ptr<PolySet>> polysets_t;

static Geometry * import_3mf_error(PolySet *mesh = nullptr, PolySet *mesh2 = nullptr)
{
	if (mesh) {
		delete mesh;
	}
	if (mesh2) {
		delete mesh2;
	}
	return new PolySet(3);
}

Geometry *import_3mf(const std::string &filename, const Location &loc)
{
	Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
	Lib3MF::PWrapper wrapper;
	
	try {
		wrapper = Lib3MF::CWrapper::loadLibrary();
		wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
		if (interfaceVersionMajor != LIB3MF_VERSION_MAJOR) {
			LOG(message_group::Error,Location::NONE,"","Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
	                        interfaceVersionMajor,interfaceVersionMinor,interfaceVersionMicro,
	                        LIB3MF_VERSION_MAJOR,LIB3MF_VERSION_MINOR,LIB3MF_VERSION_MICRO);
			return new PolySet(3);
		}
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
		return new PolySet(3);
	}

	Lib3MF::PModel model;
	try {
		model = wrapper->CreateModel();
		if (!model) {
			LOG(message_group::Error,Location::NONE,"","Could not create model");
			return new PolySet(3);		
		}
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
		return new PolySet(3);		
	}

	Lib3MF::PReader reader;
	try {
		reader = model->QueryReader("3mf");
		if (!reader) {
			LOG(message_group::Error,Location::NONE,"","Could not create 3MF reader");
			return new PolySet(3);		
		}
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
		return new PolySet(3);		
	}

	bool read_error = false;
	try {
		reader->ReadFromFile(filename);
	} catch (Lib3MF::ELib3MFException &e) {
		read_error = true;
	}
	if (!reader || read_error) {
		LOG(message_group::Warning,Location::NONE,"","Could not read file '%1$s', import() at line %2$d",filename.c_str(),loc.firstLine());
		return new PolySet(3);		
	}

	Lib3MF::PMeshObjectIterator object_it;
	object_it = model->GetMeshObjects();
	if (!object_it) {
		return new PolySet(3);		
	}

	PolySet *first_mesh = 0;
	polysets_t meshes;
	unsigned int mesh_idx = 0;
	bool has_next = object_it->MoveNext();
	while (has_next) {
		Lib3MF::PMeshObject object;
		try {
			object = object_it->GetCurrentMeshObject();
			if (!object) {
				return import_3mf_error(first_mesh);
			}
		} catch (Lib3MF::ELib3MFException &e) {
			LOG(message_group::Error,Location::NONE,"",e.what());
			return import_3mf_error(first_mesh);			
		}

		Lib3MF_uint64 vertex_count = object->GetVertexCount();
		if (!vertex_count) {
			return import_3mf_error(first_mesh);
		}
		Lib3MF_uint64 triangle_count = object->GetTriangleCount();
		if (!triangle_count) {
			return import_3mf_error(first_mesh);
		}

		PRINTDB("%s: mesh %d, vertex count: %lu, triangle count: %lu", filename.c_str() % mesh_idx % vertex_count % triangle_count);

		PolySet *p = new PolySet(3);
		for (Lib3MF_uint64 idx = 0; idx < triangle_count; ++idx) {
			Lib3MF::sTriangle triangle = object->GetTriangle(idx);
			Lib3MF::sPosition vertex1, vertex2, vertex3;
			
			vertex1 = object->GetVertex(triangle.m_Indices[0]);
			vertex2 = object->GetVertex(triangle.m_Indices[1]);
			vertex3 = object->GetVertex(triangle.m_Indices[2]);
			
			p->append_poly();
			p->append_vertex(vertex1.m_Coordinates[0], vertex1.m_Coordinates[1], vertex1.m_Coordinates[2]);
			p->append_vertex(vertex2.m_Coordinates[0], vertex2.m_Coordinates[1], vertex2.m_Coordinates[2]);
			p->append_vertex(vertex3.m_Coordinates[0], vertex3.m_Coordinates[1], vertex3.m_Coordinates[2]);
		}

		if (first_mesh) {
			meshes.push_back(std::shared_ptr<PolySet>(p));
		} else {
			first_mesh = p;
		}
		mesh_idx++;
		has_next = object_it->MoveNext();
	}

	if (first_mesh == 0) {
		return new PolySet(3);
	} else if (meshes.empty()) {
		return first_mesh;
	} else {
		PolySet *p = new PolySet(3);
#ifdef ENABLE_CGAL
		Geometry::Geometries children;
		children.push_back(std::make_pair((const AbstractNode*)NULL,  shared_ptr<const Geometry>(first_mesh)));
		for (polysets_t::iterator it = meshes.begin(); it != meshes.end(); ++it) {
			children.push_back(std::make_pair((const AbstractNode*)NULL,  shared_ptr<const Geometry>(*it)));
		}
		CGAL_Nef_polyhedron *N = CGALUtils::applyUnion3D(children.begin(), children.end());

		CGALUtils::createPolySetFromNefPolyhedron3(*N->p3, *p);
		delete N;
#endif
		return p;
	}
}

#endif // LIB3MF_API_2

#else // ENABLE_LIB3MF

const std::string get_lib3mf_version() {
	const std::string lib3mf_version = "(not enabled)";
	return lib3mf_version;
}

Geometry * import_3mf(const std::string &, const Location &loc)
{
	LOG(message_group::Warning,Location::NONE,"","Import from 3MF format was not enabled when building the application, import() at line %1$d",loc.firstLine());
	return new PolySet(3);
}

#endif // ENABLE_LIB3MF
