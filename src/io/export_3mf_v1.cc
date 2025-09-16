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

#include "io/export.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

#include <Common/Platform/NMR_WinTypes.h>
#include <Model/COM/NMR_DLLInterfaces.h>

#include "core/ColorUtil.h"
#include "export_enums.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGALNefGeometry.h"
#endif

#undef BOOL
using namespace NMR;

using S = Settings::SettingsExport3mf;

namespace {

struct ExportContext {
  PLib3MFModel *model = nullptr;
  PLib3MFModelBaseMaterial *basematerial = nullptr;
  DWORD basematerialid = 0;
  bool usecolors = false;
  int modelcount = 0;
  Color4f defaultColor;
  DWORD defaultColorId = 0;
  std::vector<DWORD> materialids;
  const ExportInfo& info;
  const std::shared_ptr<const Export3mfOptions> options;
};

uint32_t lib3mf_write_callback(const char *data, uint32_t bytes, std::ostream *stream)
{
  stream->write(data, bytes);
  return !(*stream);
}

uint32_t lib3mf_seek_callback(uint64_t pos, std::ostream *stream)
{
  stream->seekp(pos);
  return !(*stream);
}

void export_3mf_error(std::string msg, PLib3MFModel *& model)
{
  LOG(message_group::Export_Error, std::move(msg));
  if (model) {
    lib3mf_release(model);
    model = nullptr;
  }
}

int count_mesh_objects(PLib3MFModel *& model)
{
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

bool handle_triangle_color(PLib3MFPropertyHandler *propertyhandler, const std::unique_ptr<PolySet>& ps,
                           int triangle_index, int color_index, ExportContext& ctx)
{
  if (color_index < 0) {
    return true;
  }
  if (ps->colors.empty()) {
    return true;
  }
  if (!ctx.basematerial && !ctx.usecolors) {
    return true;
  }
  if (ctx.options->colorMode == Export3mfColorMode::selected_only) {
    return true;
  }

  if (ctx.basematerial) {
    if (lib3mf_propertyhandler_setbasematerial(propertyhandler, triangle_index, ctx.basematerialid,
                                               ctx.materialids[color_index]) != LIB3MF_OK) {
      export_3mf_error("Can't set triangle base material.", ctx.model);
      return false;
    }
  } else if (ctx.usecolors) {
    const auto& col = ps->colors[color_index];
    if (lib3mf_propertyhandler_setsinglecolorfloatrgba(propertyhandler, triangle_index, col.r(), col.g(),
                                                       col.b(), col.a()) != LIB3MF_OK) {
      export_3mf_error("Can't set triangle color.", ctx.model);
      return false;
    }
  }

  return true;
}

/*
 * PolySet must be triangulated.
 */
bool append_polyset(const std::shared_ptr<const PolySet>& ps, ExportContext& ctx)
{
  PLib3MFModelMeshObject *mesh = nullptr;
  if (lib3mf_model_addmeshobject(ctx.model, &mesh) != LIB3MF_OK) {
    export_3mf_error("Can't add mesh to 3MF model.", ctx.model);
    return false;
  }

  std::string name = "OpenSCAD Model";
  std::string partname = "";

  if (ctx.modelcount > 1) {
    int mesh_count = count_mesh_objects(ctx.model);
    name += " " + std::to_string(mesh_count);
    partname += "Part " + std::to_string(mesh_count);
  }
  if (lib3mf_object_setnameutf8(mesh, name.c_str()) != LIB3MF_OK) {
    export_3mf_error("Can't set name for 3MF model.", ctx.model);
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
    uint8_t r, g, b, a;
    if (!col.getRgba(r, g, b, a)) {
      LOG(message_group::Warning, "Invalid color in 3MF export");
    }
    lib3mf_basematerial_addmaterialutf8(ctx.basematerial, colname.c_str(), r, g, b, &id);
    return id;
  };

  auto sorted_ps = createSortedPolySet(*ps);

  for (const auto& v : sorted_ps->vertices) {
    if (!vertexFunc(v)) {
      export_3mf_error("Can't add vertex to 3MF model.", ctx.model);
      return false;
    }
  }

  for (const auto& poly : sorted_ps->indices) {
    if (!triangleFunc(poly)) {
      export_3mf_error("Can't add triangle to 3MF model.", ctx.model);
      return false;
    }
  }

  DWORD materials = 0;
  if (ctx.basematerial) {
    PLib3MFModelResourceIterator *it;
    if (lib3mf_model_getbasematerials(ctx.model, &it) == LIB3MF_OK) {
      while (true) {
        BOOL hasNext = false;
        if (lib3mf_resourceiterator_movenext(it, &hasNext) != LIB3MF_OK) {
          export_3mf_error("Can't move to next base material iterator value.", ctx.model);
          return false;
        }
        if (!hasNext) {
          break;
        }

        PLib3MFModelResource *resource = nullptr;
        if (lib3mf_resourceiterator_getcurrent(it, &resource) != LIB3MF_OK) {
          export_3mf_error("Can't get current value from base material iterator.", ctx.model);
          return false;
        } else {
          DWORD count = 0;
          lib3mf_basematerial_getcount(resource, &count);
          materials = count;
        }
      }
    }

    ctx.materialids.reserve(sorted_ps->colors.size());
    for (size_t i = 0; i < sorted_ps->colors.size(); i++) {
      ctx.materialids.push_back(materialFunc(materials + i, sorted_ps->colors[i]));
    }
  }

  PLib3MFPropertyHandler *propertyhandler = nullptr;
  if (lib3mf_meshobject_createpropertyhandler(mesh, &propertyhandler) != LIB3MF_OK) {
    export_3mf_error("Can't create property handler for 3MF model.", ctx.model);
    return false;
  }

  for (size_t i = 0; i < sorted_ps->color_indices.size(); ++i) {
    const int32_t idx = sorted_ps->color_indices[i];
    if (!handle_triangle_color(propertyhandler, sorted_ps, i, idx, ctx)) {
      return false;
    }
  }

  lib3mf_release(propertyhandler);

  PLib3MFPropertyHandler *defaultpropertyhandler = nullptr;
  if (lib3mf_object_createdefaultpropertyhandler(mesh, &defaultpropertyhandler) != LIB3MF_OK) {
    export_3mf_error("Can't create default property handler for 3MF model.", ctx.model);
    return false;
  }

  if (ctx.basematerial) {
    lib3mf_defaultpropertyhandler_setbasematerial(defaultpropertyhandler, ctx.basematerialid,
                                                  ctx.defaultColorId);
  } else if (ctx.usecolors) {
    uint8_t r, g, b, a;
    if (!ctx.defaultColor.getRgba(r, g, b, a)) {
      LOG(message_group::Warning, "Invalid color in 3MF export");
    }
    lib3mf_defaultpropertyhandler_setcolorrgba(defaultpropertyhandler, r, g, b, a);
  }

  lib3mf_release(defaultpropertyhandler);

  PLib3MFModelBuildItem *builditem = nullptr;
  if (lib3mf_model_addbuilditem(ctx.model, mesh, nullptr, &builditem) != LIB3MF_OK) {
    export_3mf_error("Can't add build item to 3MF model.", ctx.model);
    return false;
  }
  if (!partname.empty() &&
      lib3mf_builditem_setpartnumberutf8(builditem, partname.c_str()) != LIB3MF_OK) {
    export_3mf_error("Can't set part name of build item.", ctx.model);
    return false;
  }

  lib3mf_release(mesh);
  lib3mf_release(builditem);

  return true;
}

#ifdef ENABLE_CGAL
bool append_nef(const CGALNefGeometry& root_N, ExportContext& ctx)
{
  if (!root_N.p3) {
    LOG(message_group::Export_Error, "Export failed, empty geometry.");
    return false;
  }

  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning,
        "Exported object may not be a valid 2-manifold and may need repair");
  }

  if (std::shared_ptr<PolySet> ps = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3)) {
    return append_polyset(ps, ctx);
  }

  export_3mf_error("Error converting NEF Polyhedron.", ctx.model);
  return false;
}
#endif  // ifdef ENABLE_CGAL

static bool append_3mf(const std::shared_ptr<const Geometry>& geom, ExportContext& ctx)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    ctx.modelcount = geomlist->getChildren().size();
    for (const auto& item : geomlist->getChildren()) {
      if (!append_3mf(item.second, ctx)) return false;
    }
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGALNefGeometry>(geom)) {
    return append_nef(*N, ctx);
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return append_polyset(mani->toPolySet(), ctx);
#endif
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return append_polyset(PolySetUtils::tessellate_faces(*ps), ctx);
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) {  // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else {  // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

  return true;
}

void add_meta_data(PLib3MFModelMeshObject *& model, const std::string& name, const std::string& value,
                   const std::string& value2 = "")
{
  const std::string v = value.empty() ? value2 : value;
  if (v.empty()) {
    return;
  }

  lib3mf_model_addmetadatautf8(model, name.c_str(), v.c_str());
}

}  // namespace

/*!
    Saves the current 3D Geometry as 3MF to the given file.
    The file must be open.
 */
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo)
{
  DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  HRESULT result =
    lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
  if (result != LIB3MF_OK) {
    LOG(message_group::Export_Error, "Error reading 3MF library version");
    return;
  }

  if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
    LOG(message_group::Export_Error,
        "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
        interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro,
        NMR_APIVERSION_INTERFACE_MAJOR, NMR_APIVERSION_INTERFACE_MINOR, NMR_APIVERSION_INTERFACE_MICRO);
    return;
  }

  PLib3MFModel *model;
  result = lib3mf_createmodel(&model);
  if (result != LIB3MF_OK) {
    LOG(message_group::Export_Error, "Can't create 3MF model.");
    return;
  }
  const auto& options3mf =
    exportInfo.options3mf ? exportInfo.options3mf : std::make_shared<Export3mfOptions>();
  switch (options3mf->unit) {
  case Export3mfUnit::micron:     lib3mf_model_setunit(model, eModelUnit::MODELUNIT_MICROMETER); break;
  case Export3mfUnit::centimeter: lib3mf_model_setunit(model, eModelUnit::MODELUNIT_CENTIMETER); break;
  case Export3mfUnit::meter:      lib3mf_model_setunit(model, eModelUnit::MODELUNIT_METER); break;
  case Export3mfUnit::inch:       lib3mf_model_setunit(model, eModelUnit::MODELUNIT_INCH); break;
  case Export3mfUnit::foot:       lib3mf_model_setunit(model, eModelUnit::MODELUNIT_FOOT); break;
  default:                        lib3mf_model_setunit(model, eModelUnit::MODELUNIT_MILLIMETER); break;
  }

  Color4f defaultColor;
  DWORD defaultColorId = 0;

  bool usecolors = false;
  DWORD basematerialid = 0;
  PLib3MFModelBaseMaterial *basematerial = nullptr;
  if (options3mf->colorMode != Export3mfColorMode::none) {
    if (options3mf->colorMode == Export3mfColorMode::model) {
      // use default color that ultimately should come from the color scheme
      defaultColor = exportInfo.defaultColor;
    } else {
      defaultColor = OpenSCAD::getColor(options3mf->color, exportInfo.defaultColor);
    }
    if (options3mf->materialType == Export3mfMaterialType::basematerial) {
      if (lib3mf_model_addbasematerialgroup(model, &basematerial) != LIB3MF_OK) {
        export_3mf_error("Can't create base material group.", model);
        return;
      }
      if (lib3mf_resource_getresourceid(basematerial, &basematerialid) != LIB3MF_OK) {
        export_3mf_error("Can't get base material resource id.", model);
        return;
      }
      uint8_t r, g, b, a;
      if (!defaultColor.getRgba(r, g, b, a)) {
        LOG(message_group::Warning, "Invalid color in 3MF export");
      }
      if (lib3mf_basematerial_addmaterialutf8(basematerial, "Default", r, g, b, &defaultColorId) !=
          LIB3MF_OK) {
        export_3mf_error("Can't add default material color.", model);
        return;
      }
    } else if (options3mf->materialType == Export3mfMaterialType::color) {
      usecolors = true;
    }
  }

  if (options3mf->addMetaData) {
    add_meta_data(model, "Title", options3mf->metaDataTitle, exportInfo.title);
    add_meta_data(model, "Application", EXPORT_CREATOR);
    add_meta_data(model, "CreationDate", get_current_iso8601_date_time_utc());
    add_meta_data(model, "Designer", options3mf->metaDataDesigner);
    add_meta_data(model, "Description", options3mf->metaDataDescription);
    add_meta_data(model, "Copyright", options3mf->metaDataCopyright);
    add_meta_data(model, "LicenseTerms", options3mf->metaDataLicenseTerms);
    add_meta_data(model, "Rating", options3mf->metaDataRating);
  }

  ExportContext ctx{.model = model,
                    .basematerial = basematerial,
                    .basematerialid = basematerialid,
                    .usecolors = usecolors,
                    .modelcount = 1,
                    .defaultColor = defaultColor,
                    .defaultColorId = defaultColorId,
                    .info = exportInfo,
                    .options = options3mf};

  if (!append_3mf(geom, ctx)) {
    if (ctx.model) lib3mf_release(model);
    return;
  }

  PLib3MFModelWriter *writer;
  if (lib3mf_model_querywriter(model, "3mf", &writer) != LIB3MF_OK) {
    export_3mf_error("Can't get writer for 3MF model.", model);
    return;
  }

  result = lib3mf_writer_writetocallback(writer, (void *)lib3mf_write_callback,
                                         (void *)lib3mf_seek_callback, &output);
  output.flush();
  lib3mf_release(writer);
  lib3mf_release(model);
  if (result != LIB3MF_OK) {
    LOG(message_group::Export_Error, "Error writing 3MF model.");
  }
}
