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

#ifndef LIB3MF_API_2
#include <Model/COM/NMR_DLLInterfaces.h>
#undef BOOL
using namespace NMR;

#include <algorithm>

#ifdef ENABLE_CGAL

#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

static void export_3mf_error(const std::string msg, PLib3MFModel *&model)
{
	LOG(message_group::Export_Error,Location::NONE,"",std::string(msg));
	if (model) {
		lib3mf_release(model);
		model = nullptr;
	}
}

/*
 * PolySet must be triangulated.
 */
static bool append_polyset(const PolySet &ps, PLib3MFModelMeshObject *&model)
{
	PLib3MFModelMeshObject *mesh;
	if (lib3mf_model_addmeshobject(model, &mesh) != LIB3MF_OK) {
		export_3mf_error("Can't add mesh to 3MF model.", model);
		return false;
	}
	if (lib3mf_object_setnameutf8(mesh, "OpenSCAD Model") != LIB3MF_OK) {
		export_3mf_error("Can't set name for 3MF model.", model);
		return false;
	}

	auto vertexFunc = [&](const std::array<double, 3>& coords) -> bool {
		MODELMESHVERTEX v{(FLOAT)coords[0], (FLOAT)coords[1], (FLOAT)coords[2]};
		return lib3mf_meshobject_addvertex(mesh, &v, nullptr) == LIB3MF_OK;
	};

	auto triangleFunc = [&](const std::array<int, 3>& indices) -> bool {
		MODELMESHTRIANGLE t{(DWORD)indices[0], (DWORD)indices[1], (DWORD)indices[2]};
		return lib3mf_meshobject_addtriangle(mesh, &t, nullptr) == LIB3MF_OK;
	};

	Export::ExportMesh exportMesh{ps};

	if (!exportMesh.foreach_vertex(vertexFunc)) {
		export_3mf_error("Can't add vertex to 3MF model.", model);
		return false;
	}

	if (!exportMesh.foreach_triangle(triangleFunc)) {
		export_3mf_error("Can't add triangle to 3MF model.", model);
		return false;
	}

	PLib3MFModelBuildItem *builditem;
	if (lib3mf_model_addbuilditem(model, mesh, nullptr, &builditem) != LIB3MF_OK) {
		export_3mf_error("Can't add build item to 3MF model.", model);
		return false;
	}

	return true;
}

static bool append_nef(const CGAL_Nef_polyhedron &root_N, PLib3MFModelMeshObject *&model)
{
	if (!root_N.p3) {
		LOG(message_group::Export_Error,Location::NONE,"","Export failed, empty geometry.");
		return false;
	}

	if (!root_N.p3->is_simple()) {
		LOG(message_group::Export_Warning,Location::NONE,"","Exported object may not be a valid 2-manifold and may need repair");
	}

	PolySet ps{3};
	const bool err = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3, ps);
	if (err) {
		export_3mf_error("Error converting NEF Polyhedron.", model);
		return false;
	}

	return append_polyset(ps, model);
}

static bool append_3mf(const shared_ptr<const Geometry> &geom, PLib3MFModelMeshObject *&model)
{
	if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		for (const auto &item : geomlist->getChildren()) {
			if (!append_3mf(item.second, model)) return false;
		}
	}
	else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		return append_nef(*N, model);
	}
	else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
		PolySet triangulated(3);
		PolysetUtils::tessellate_faces(*ps, triangulated);
		return append_polyset(triangulated, model);
	}
	else if (dynamic_pointer_cast<const Polygon2d>(geom)) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}

	return true;
}

/*!
    Saves the current 3D Geometry as 3MF to the given file.
    The file must be open.
 */
void export_3mf(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
	HRESULT result = lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
	if (result != LIB3MF_OK) {
		LOG(message_group::Export_Error,Location::NONE,"","Error reading 3MF library version");
		return;
	}

	if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
		LOG(message_group::Export_Error,Location::NONE,"","Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",interfaceVersionMajor,interfaceVersionMinor,interfaceVersionMicro,NMR_APIVERSION_INTERFACE_MAJOR,NMR_APIVERSION_INTERFACE_MINOR,NMR_APIVERSION_INTERFACE_MICRO);
		return;
	}

	PLib3MFModel *model;
	result = lib3mf_createmodel(&model);
	if (result != LIB3MF_OK) {
		LOG(message_group::Export_Error,Location::NONE,"","Can't create 3MF model.");
		return;
	}

	if (!append_3mf(geom, model)) {
		if (model) lib3mf_release(model);
		return;
	}

	PLib3MFModelWriter *writer;
	if (lib3mf_model_querywriter(model, "3mf", &writer) != LIB3MF_OK) {
		export_3mf_error("Can't get writer for 3MF model.", model);
		return;
	}

	result = lib3mf_writer_writetocallback(writer, (void *)lib3mf_write_callback, (void *)lib3mf_seek_callback, &output);
	output.flush();
	lib3mf_release(writer);
	lib3mf_release(model);
	if (result != LIB3MF_OK) {
		LOG(message_group::Export_Error,Location::NONE,"","Error writing 3MF model.");
	}


}

#endif // ENABLE_CGAL

#else // LIB3MF_API_2

#include "lib3mf_implicit.hpp"

#include <algorithm>

#ifdef ENABLE_CGAL

#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

static void export_3mf_error(const std::string msg)
{
	LOG(message_group::Export_Error,Location::NONE,"",std::string(msg));
}

/*
 * PolySet must be triangulated.
 */
static bool append_polyset(const PolySet &ps, Lib3MF::PWrapper &wrapper, Lib3MF::PModel &model)
{
	try {
		auto mesh = model->AddMeshObject();
		if (!mesh) return false;
		mesh->SetName("OpenSCAD Model");

		auto vertexFunc = [&](const std::array<double, 3>& coords) -> bool {
			try {
				Lib3MF::sPosition v{(Lib3MF_single)coords[0], (Lib3MF_single)coords[1], (Lib3MF_single)coords[2]};
				mesh->AddVertex(v);
			} catch (Lib3MF::ELib3MFException &e) {
				export_3mf_error(e.what());
				return false;
			}
			return true;
		};

		auto triangleFunc = [&](const std::array<int, 3>& indices) -> bool {
			try {
				Lib3MF::sTriangle t{(Lib3MF_uint32)indices[0], (Lib3MF_uint32)indices[1], (Lib3MF_uint32)indices[2]};
				mesh->AddTriangle(t);
			} catch (Lib3MF::ELib3MFException &e) {
				export_3mf_error(e.what());
				return false;
			}
			return true;
		};

		Export::ExportMesh exportMesh{ps};

		if (!exportMesh.foreach_vertex(vertexFunc)) {
			export_3mf_error("Can't add vertex to 3MF model.");
			return false;
		}

		if (!exportMesh.foreach_triangle(triangleFunc)) {
			export_3mf_error("Can't add triangle to 3MF model.");
			return false;
		}

		Lib3MF::PBuildItem builditem;
		try {
			model->AddBuildItem(mesh.get(), wrapper->GetIdentityTransform());
		} catch (Lib3MF::ELib3MFException &e) {
			export_3mf_error(e.what());
		}
	} catch (Lib3MF::ELib3MFException &e) {
		export_3mf_error(e.what());
		return false;
	}
	return true;
}

static bool append_nef(const CGAL_Nef_polyhedron &root_N, Lib3MF::PWrapper &wrapper, Lib3MF::PModel &model)
{
	if (!root_N.p3) {
		LOG(message_group::Export_Error,Location::NONE,"","Export failed, empty geometry.");
		return false;
	}

	if (!root_N.p3->is_simple()) {
		LOG(message_group::Export_Warning,Location::NONE,"","Exported object may not be a valid 2-manifold and may need repair");
	}

	PolySet ps{3};
	const bool err = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3, ps);
	if (err) {
		export_3mf_error("Error converting NEF Polyhedron.");
		return false;
	}

	return append_polyset(ps, wrapper, model);
}

static bool append_3mf(const shared_ptr<const Geometry> &geom, Lib3MF::PWrapper &wrapper, Lib3MF::PModel &model)
{
	if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		for (const auto &item : geomlist->getChildren()) {
			if (!append_3mf(item.second, wrapper, model)) return false;
		}
	}
	else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		return append_nef(*N, wrapper, model);
	}
	else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
		PolySet triangulated(3);
		PolysetUtils::tessellate_faces(*ps, triangulated);
		return append_polyset(triangulated, wrapper, model);
	}
	else if (dynamic_pointer_cast<const Polygon2d>(geom)) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}

	return true;
}

/*!
    Saves the current 3D Geometry as 3MF to the given file.
    The file must be open.
 */

void export_3mf(const shared_ptr<const Geometry> &geom, std::ostream &output)
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
			return;
		}
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
		return;
	}

	if ((interfaceVersionMajor != LIB3MF_VERSION_MAJOR)) {
		LOG(message_group::Export_Error,Location::NONE,"","Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",interfaceVersionMajor,interfaceVersionMinor,interfaceVersionMicro,LIB3MF_VERSION_MAJOR,LIB3MF_VERSION_MINOR,LIB3MF_VERSION_MICRO);
		return;
	}

	Lib3MF::PModel model;
	try {
		model = wrapper->CreateModel();
		if (!model) {
			LOG(message_group::Export_Error,Location::NONE,"","Can't create 3MF model.");
			return;
		}
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
		return;
	}
	
	if (!append_3mf(geom, wrapper, model)) {
		return;
	}

	Lib3MF::PWriter writer;
	try {
		writer = model->QueryWriter("3mf");
		if (!writer) {
			export_3mf_error("Can't get writer for 3MF model.");
			return;
		}
	} catch (Lib3MF::ELib3MFException &e) {
		export_3mf_error("Can't get writer for 3MF model.");
		return;
	}

	try {
		writer->WriteToCallback((Lib3MF::WriteCallback)lib3mf_write_callback, (Lib3MF::SeekCallback)lib3mf_seek_callback, &output);
	} catch (Lib3MF::ELib3MFException &e) {
		LOG(message_group::Export_Error,Location::NONE,"",e.what());
	}
	output.flush();
}

#endif // ENABLE_CGAL

#endif // LIB3MF_API_2

#else // ENABLE_LIB3MF

void export_3mf(const shared_ptr<const Geometry> &, std::ostream &)
{
	LOG(message_group::None,Location::NONE,"","Export to 3MF format was not enabled when building the application.");
}

#endif // ENABLE_LIB3MF
