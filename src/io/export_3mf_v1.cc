/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2024 Clifford Wolf <clifford@clifford.at> and
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

#include "geometry/GeometryUtils.h"
#include "io/export.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include <cassert>
#include <ostream>
#include <cstdint>
#include <memory>
#include <string>
#include <algorithm>

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

#include <Model/COM/NMR_DLLInterfaces.h>
#undef BOOL
using namespace NMR;

#include <algorithm>

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#endif

namespace {

void export_3mf_error(std::string msg, PLib3MFModel *& model)
{
  LOG(message_group::Export_Error, std::move(msg));
  if (model) {
    lib3mf_release(model);
    model = nullptr;
  }
}

BYTE get_color_channel(const Color4f& col, int idx)
{
  return std::clamp(static_cast<int>(255.0 * col[idx]), 0, 255);
}

int count_mesh_objects(PLib3MFModel *& model) {
    PLib3MFModelResourceIterator *it;
    if (lib3mf_model_getmeshobjects(model, &it) != LIB3MF_OK) {
      return 0;
    }

    BOOL hasNext;
    int count = 0;
    while (true) {
      if (lib3mf_resourceiterator_movenext(it, &hasNext) != LIB3MF_OK) {
          return 0;
      }
      if (!hasNext) {
        break;
      }
       ++count;
    }

    return count;
}

} // namespace

/*
 * PolySet must be triangulated.
 */
static bool append_polyset(std::shared_ptr<const PolySet> ps, PLib3MFModelMeshObject *& model, PLib3MFModelBaseMaterial *basematerial, int count)
{
  PLib3MFModelMeshObject *mesh = nullptr;
  PLib3MFPropertyHandler *propertyhandler = nullptr;
  PLib3MFPropertyHandler *defaultpropertyhandler = nullptr;
  if (lib3mf_model_addmeshobject(model, &mesh) != LIB3MF_OK) {
    export_3mf_error("Can't add mesh to 3MF model.", model);
    return false;
  }

  std::string name = "OpenSCAD Model";
  std::string partname = "";

  if (count > 1) {
    int mesh_count = count_mesh_objects(model);
    name += " " + std::to_string(mesh_count);
    partname += "Part " + std::to_string(mesh_count);
  }
  if (lib3mf_object_setnameutf8(mesh, name.c_str()) != LIB3MF_OK) {
    export_3mf_error("Can't set name for 3MF model.", model);
    return false;
  }
  
  auto vertexFunc = [&](const Vector3d& coords) -> bool {
    const auto f = coords.cast<float>();
    MODELMESHVERTEX v{f[0], f[1], f[2]};
    return lib3mf_meshobject_addvertex(mesh, &v, nullptr) == LIB3MF_OK;
  };

  auto triangleFunc = [&](const IndexedFace& indices) -> bool {
    MODELMESHTRIANGLE t{(DWORD)indices[0], (DWORD)indices[1], (DWORD)indices[2]};
    return lib3mf_meshobject_addtriangle(mesh, &t, nullptr) == LIB3MF_OK;
  };

  auto materialFunc = [&](int idx, const Color4f& col) -> DWORD {
    const auto colname = "Color " + std::to_string(idx);

    DWORD id = 0;
    lib3mf_basematerial_addmaterialutf8(basematerial,
      colname.c_str(),
      get_color_channel(col, 0),
      get_color_channel(col, 1),
      get_color_channel(col, 2),
      &id);
    return id;
  };

  auto sorted_ps = createSortedPolySet(*ps);

  for (const auto &v : sorted_ps->vertices) {
    if (!vertexFunc(v)) {
      export_3mf_error("Can't add vertex to 3MF model.", model);
      return false;
    }
  }

  for (const auto& poly : sorted_ps->indices) {
    if (!triangleFunc(poly)) {
      export_3mf_error("Can't add triangle to 3MF model.", model);
      return false;
    }
  }

	if (lib3mf_meshobject_createpropertyhandler(mesh, &propertyhandler) != LIB3MF_OK) {
    export_3mf_error("Can't create property handler for 3MF model.", model);
    return false;
  }

  DWORD basematerialid = 0;
  if (lib3mf_resource_getresourceid(basematerial, &basematerialid) != LIB3MF_OK) {
    export_3mf_error("Can't get base material resource id.", model);
    return false;
  }

  DWORD materials = 0;
  PLib3MFModelResourceIterator *it;
  if (lib3mf_model_getbasematerials(model, &it) == LIB3MF_OK) {
    while (true) {
      BOOL hasNext = false;
      if (lib3mf_resourceiterator_movenext(it, &hasNext) != LIB3MF_OK) {
        export_3mf_error("Can't move to next base material iterator value.", model);
        return false;
      }
      if (!hasNext) {
        break;
      }

      PLib3MFModelResource *resource = nullptr;
      if (lib3mf_resourceiterator_getcurrent(it, &resource) != LIB3MF_OK) {
        export_3mf_error("Can't get current value from base material iterator.", model);
        return false;
      } else {
        DWORD count = 0;
        lib3mf_basematerial_getcount(resource, &count);
        materials = count;
      }
    };
  }

  std::vector<DWORD> materialids;
  materialids.reserve(sorted_ps->colors.size());
  for (int i = 0;i < sorted_ps->colors.size();i++) {
    materialids.push_back(materialFunc(materials + i, sorted_ps->colors[i]));
  }

  if (materialids.size() > 0) {
    for(int i = 0;i < sorted_ps->color_indices.size();i++) {
      int32_t idx = sorted_ps->color_indices[i];
      if (idx < 0)
        continue;
      lib3mf_propertyhandler_setbasematerial(propertyhandler, i, basematerialid, materialids[idx]);
    }
  }

  lib3mf_release(propertyhandler);

	if (lib3mf_object_createdefaultpropertyhandler(mesh, &defaultpropertyhandler) != LIB3MF_OK) {
    export_3mf_error("Can't create default property handler for 3MF model.", model);
    return false;
  }
	lib3mf_defaultpropertyhandler_setbasematerial(defaultpropertyhandler, basematerialid, 0);
  lib3mf_release(defaultpropertyhandler);

  PLib3MFModelBuildItem *builditem = nullptr;
  if (lib3mf_model_addbuilditem(model, mesh, nullptr, &builditem) != LIB3MF_OK) {
    export_3mf_error("Can't add build item to 3MF model.", model);
    return false;
  }
  if (!partname.empty() && lib3mf_builditem_setpartnumberutf8(builditem, partname.c_str()) != LIB3MF_OK) {
    export_3mf_error("Can't set part name of build item.", model);
    return false;
  }

  return true;
}

#ifdef ENABLE_CGAL
static bool append_nef(const CGAL_Nef_polyhedron& root_N, PLib3MFModelMeshObject *& model, PLib3MFModelBaseMaterial *basematerial, int count)
{
  if (!root_N.p3) {
    LOG(message_group::Export_Error, "Export failed, empty geometry.");
    return false;
  }

  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }


  if (std::shared_ptr<PolySet> ps = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3)) {
    return append_polyset(ps, model, basematerial, count);
  }

  export_3mf_error("Error converting NEF Polyhedron.", model);
  return false;
}
#endif

static bool append_3mf(const std::shared_ptr<const Geometry>& geom, PLib3MFModelMeshObject *& model, PLib3MFModelBaseMaterial *basematerial, int count)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      if (!append_3mf(item.second, model, basematerial, geomlist->getChildren().size())) return false;
    }
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    return append_nef(*N, model, basematerial, count);
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return append_polyset(mani->toPolySet(), model, basematerial, count);
#endif
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return append_polyset(PolySetUtils::tessellate_faces(*ps), model, basematerial, count);
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

  return true;
}

/*!
    Saves the current 3D Geometry as 3MF to the given file.
    The file must be open.
 */
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo)
{
  DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  HRESULT result = lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
  if (result != LIB3MF_OK) {
    LOG(message_group::Export_Error, "Error reading 3MF library version");
    return;
  }

  if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
    LOG(message_group::Export_Error, "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d", interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro, NMR_APIVERSION_INTERFACE_MAJOR, NMR_APIVERSION_INTERFACE_MINOR, NMR_APIVERSION_INTERFACE_MICRO);
    return;
  }

  PLib3MFModel *model;
  result = lib3mf_createmodel(&model);
  if (result != LIB3MF_OK) {
    LOG(message_group::Export_Error, "Can't create 3MF model.");
    return;
  }

  PLib3MFModelBaseMaterial *basematerial = nullptr;
  if (lib3mf_model_addbasematerialgroup(model, &basematerial) != LIB3MF_OK) {
    export_3mf_error("Can't create base material group.", model);
    return;
  }

  DWORD id = 0;
  const auto dc = exportInfo.defaultColor;
  if (lib3mf_basematerial_addmaterialutf8(basematerial, "Default",
    get_color_channel(exportInfo.defaultColor, 0),
    get_color_channel(exportInfo.defaultColor, 1),
    get_color_channel(exportInfo.defaultColor, 2),
    &id) != LIB3MF_OK) {
    export_3mf_error("Can't add default material color.", model);
    return;
  }

  const std::array<std::array<std::string, 2>, 3> meta_data_list = {{
    {{ "Title", exportInfo.title }},
    {{ "Application", EXPORT_CREATOR }},
    {{ "CreationDate", get_current_iso8601_date_time_utc() }}
  }};
  for (const auto& meta_data : meta_data_list) {
    lib3mf_model_addmetadatautf8(model, meta_data[0].c_str(), meta_data[1].c_str());
  }

  if (!append_3mf(geom, model, basematerial, 1)) {
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
    LOG(message_group::Export_Error, "Error writing 3MF model.");
  }


}
