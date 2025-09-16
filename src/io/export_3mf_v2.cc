/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2025 Clifford Wolf <clifford@clifford.at> and
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
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>

#include <lib3mf_implicit.hpp>

#include "export_enums.h"
#include "Feature.h"
#include "core/ColorUtil.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryUtils.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGALNefGeometry.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

using ExportColorMap = std::unordered_map<Color4f, Lib3MF_uint32>;

namespace {

struct ExportContext {
  Lib3MF::PWrapper wrapper;
  Lib3MF::PModel model;
  Lib3MF::PColorGroup colorgroup;
  Lib3MF::PBaseMaterialGroup basematerialgroup;
  int modelcount;
  ExportColorMap colors;
  Color4f selectedColor;
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

void export_3mf_error(std::string msg) { LOG(message_group::Export_Error, std::move(msg)); }

int count_mesh_objects(const Lib3MF::PModel& model)
{
  const auto mesh_object_it = model->GetMeshObjects();
  int count = 0;
  while (mesh_object_it->MoveNext()) ++count;
  return count;
}

void handle_triangle_color(const std::shared_ptr<const PolySet>& ps, ExportContext& ctx,
                           Lib3MF::PMeshObject& mesh, Lib3MF_uint32 triangle, int color_index)
{
  if (color_index < 0) {
    return;
  }
  if (ps->colors.empty()) {
    return;
  }
  if (!ctx.basematerialgroup && !ctx.colorgroup) {
    return;
  }
  if (ctx.options->colorMode == Export3mfColorMode::selected_only) {
    return;
  }

  const Color4f col = ps->colors[color_index];
  const auto col_it = ctx.colors.find(col);

  Lib3MF_uint32 col_idx = 0;
  if (col_it == ctx.colors.end()) {
    Lib3MF::sColor materialcolor;
    if (!col.getRgba(materialcolor.m_Red, materialcolor.m_Green, materialcolor.m_Blue,
                     materialcolor.m_Alpha)) {
      LOG(message_group::Warning, "Invalid color in 3MF export");
    }
    if (ctx.basematerialgroup) {
      col_idx = ctx.basematerialgroup->AddMaterial(
        "Color " + std::to_string(ctx.basematerialgroup->GetCount()), materialcolor);
    } else if (ctx.colorgroup) {
      col_idx = ctx.colorgroup->AddColor(materialcolor);
    }
    ctx.colors[col] = col_idx;
  } else {
    col_idx = (*col_it).second;
  }

  Lib3MF_uint32 res_id = 0;
  if (ctx.basematerialgroup) {
    res_id = ctx.basematerialgroup->GetUniqueResourceID();
  } else if (ctx.colorgroup) {
    res_id = ctx.colorgroup->GetUniqueResourceID();
  }

  if (res_id > 0) {
    mesh->SetTriangleProperties(triangle, {res_id, {col_idx, col_idx, col_idx}});
  }
}

/*
 * PolySet must be triangulated.
 */
bool append_polyset(const std::shared_ptr<const PolySet>& ps, ExportContext& ctx)
{
  try {
    auto mesh = ctx.model->AddMeshObject();
    if (!mesh) return false;

    const int mesh_count = count_mesh_objects(ctx.model);
    const auto modelname =
      ctx.modelcount == 1 ? "OpenSCAD Model" : "OpenSCAD Model " + std::to_string(mesh_count);
    const auto partname = ctx.modelcount == 1 ? "" : "Part " + std::to_string(mesh_count);
    mesh->SetName(modelname);
    if (ctx.basematerialgroup) {
      mesh->SetObjectLevelProperty(ctx.basematerialgroup->GetUniqueResourceID(), 1);
    } else if (ctx.colorgroup) {
      mesh->SetObjectLevelProperty(ctx.colorgroup->GetUniqueResourceID(), 1);
    }

    auto vertexFunc = [&](const Vector3d& coords) -> bool {
      const auto f = coords.cast<float>();
      try {
        const Lib3MF::sPosition v{f[0], f[1], f[2]};
        mesh->AddVertex(v);
      } catch (Lib3MF::ELib3MFException& e) {
        export_3mf_error(e.what());
        return false;
      }
      return true;
    };

    auto triangleFunc = [&](const IndexedFace& indices, int color_index) -> bool {
      try {
        const auto triangle = mesh->AddTriangle({static_cast<Lib3MF_uint32>(indices[0]),
                                                 static_cast<Lib3MF_uint32>(indices[1]),
                                                 static_cast<Lib3MF_uint32>(indices[2])});

        handle_triangle_color(ps, ctx, mesh, triangle, color_index);
      } catch (Lib3MF::ELib3MFException& e) {
        export_3mf_error(e.what());
        return false;
      }
      return true;
    };

    std::shared_ptr<const PolySet> out_ps = ps;
    if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
      out_ps = createSortedPolySet(*ps);
    }

    for (const auto& v : out_ps->vertices) {
      if (!vertexFunc(v)) {
        export_3mf_error("Can't add vertex to 3MF model.");
        return false;
      }
    }

    for (size_t i = 0; i < out_ps->indices.size(); i++) {
      auto color_index = i < out_ps->color_indices.size() ? out_ps->color_indices[i] : -1;
      if (!triangleFunc(out_ps->indices[i], color_index)) {
        export_3mf_error("Can't add triangle to 3MF model.");
        return false;
      }
    }

    try {
      auto builditem = ctx.model->AddBuildItem(mesh.get(), ctx.wrapper->GetIdentityTransform());
      if (!partname.empty()) {
        builditem->SetPartNumber(partname);
      }
    } catch (Lib3MF::ELib3MFException& e) {
      export_3mf_error(e.what());
    }
  } catch (Lib3MF::ELib3MFException& e) {
    export_3mf_error(e.what());
    return false;
  }
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

  if (const std::shared_ptr<const PolySet> ps = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3)) {
    return append_polyset(ps, ctx);
  }
  export_3mf_error("Error converting NEF Polyhedron.");
  return false;
}
#endif  // ifdef ENABLE_CGAL

bool append_3mf(const std::shared_ptr<const Geometry>& geom, ExportContext& ctx)
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
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    assert(false && "Unsupported file format");
  } else {
    assert(false && "Not implemented");
  }

  return true;
}

void add_meta_data(Lib3MF::PMetaDataGroup& metadatagroup, const std::string& name,
                   const std::string& value, const std::string& value2 = "")
{
  const std::string v = value.empty() ? value2 : value;
  if (v.empty()) {
    return;
  }

  metadatagroup->AddMetaData("", name, v, "xs:string", true);
}

}  // namespace

/*!
    Saves the current 3D Geometry as 3MF to the given file.
    The file must be open.
 */
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo)
{
  Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  Lib3MF::PWrapper wrapper;

  try {
    wrapper = Lib3MF::CWrapper::loadLibrary();
    wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
    if (interfaceVersionMajor != LIB3MF_VERSION_MAJOR) {
      LOG(message_group::Error,
          "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
          interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro, LIB3MF_VERSION_MAJOR,
          LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO);
      return;
    }
  } catch (Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
    return;
  }

  if ((interfaceVersionMajor != LIB3MF_VERSION_MAJOR)) {
    LOG(message_group::Export_Error,
        "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
        interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro, LIB3MF_VERSION_MAJOR,
        LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO);
    return;
  }

  Lib3MF::PModel model;
  try {
    model = wrapper->CreateModel();
    if (!model) {
      LOG(message_group::Export_Error, "Can't create 3MF model.");
      return;
    }
  } catch (Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
    return;
  }

  const auto& options3mf =
    exportInfo.options3mf ? exportInfo.options3mf : std::make_shared<Export3mfOptions>();
  switch (options3mf->unit) {
  case Export3mfUnit::micron:     model->SetUnit(Lib3MF::eModelUnit::MicroMeter); break;
  case Export3mfUnit::centimeter: model->SetUnit(Lib3MF::eModelUnit::CentiMeter); break;
  case Export3mfUnit::meter:      model->SetUnit(Lib3MF::eModelUnit::Meter); break;
  case Export3mfUnit::inch:       model->SetUnit(Lib3MF::eModelUnit::Inch); break;
  case Export3mfUnit::foot:       model->SetUnit(Lib3MF::eModelUnit::Foot); break;
  default:                        model->SetUnit(Lib3MF::eModelUnit::MilliMeter); break;
  }

  // use default color that ultimately should come from the color scheme
  Color4f color = exportInfo.defaultColor;

  Lib3MF::PColorGroup colorgroup;
  Lib3MF::PBaseMaterialGroup basematerialgroup;
  if (options3mf->colorMode != Export3mfColorMode::none) {
    if (options3mf->colorMode != Export3mfColorMode::model) {
      // use color selected in the export dialog and stored in settings (if valid)
      color = OpenSCAD::getColor(options3mf->color, exportInfo.defaultColor);
    }
    if (options3mf->materialType == Export3mfMaterialType::basematerial) {
      basematerialgroup = model->AddBaseMaterialGroup();
      Lib3MF::sColor materialcolor;
      if (!color.getRgba(materialcolor.m_Red, materialcolor.m_Green, materialcolor.m_Blue,
                         materialcolor.m_Alpha)) {
        LOG(message_group::Warning, "Invalid color in 3MF export");
      }
      materialcolor.m_Alpha = 0xff;
      basematerialgroup->AddMaterial("Default", materialcolor);
    } else if (options3mf->materialType == Export3mfMaterialType::color) {
      colorgroup = model->AddColorGroup();
      Lib3MF::sColor groupcolor;
      if (!color.getRgba(groupcolor.m_Red, groupcolor.m_Green, groupcolor.m_Blue, groupcolor.m_Alpha)) {
        LOG(message_group::Warning, "Invalid color in 3MF export");
      }
      colorgroup->AddColor(groupcolor);
    }
  }

  if (options3mf->addMetaData) {
    auto metadatagroup = model->GetMetaDataGroup();
    add_meta_data(metadatagroup, "Title", options3mf->metaDataTitle, exportInfo.title);
    add_meta_data(metadatagroup, "Application", EXPORT_CREATOR);
    add_meta_data(metadatagroup, "CreationDate", get_current_iso8601_date_time_utc());
    add_meta_data(metadatagroup, "Designer", options3mf->metaDataDesigner);
    add_meta_data(metadatagroup, "Description", options3mf->metaDataDescription);
    add_meta_data(metadatagroup, "Copyright", options3mf->metaDataCopyright);
    add_meta_data(metadatagroup, "LicenseTerms", options3mf->metaDataLicenseTerms);
    add_meta_data(metadatagroup, "Rating", options3mf->metaDataRating);
  }

  ExportContext ctx{.wrapper = wrapper,
                    .model = model,
                    .colorgroup = colorgroup,
                    .basematerialgroup = basematerialgroup,
                    .modelcount = 1,
                    .selectedColor = color,
                    .info = exportInfo,
                    .options = options3mf};

  if (!append_3mf(geom, ctx)) {
    return;
  }

  Lib3MF::PWriter writer;
  try {
    writer = model->QueryWriter("3mf");
    if (!writer) {
      export_3mf_error("Can't get writer for 3MF model.");
      return;
    }
  } catch (Lib3MF::ELib3MFException& e) {
    export_3mf_error("Can't get writer for 3MF model.");
    return;
  }

  try {
    writer->SetDecimalPrecision(ctx.options->decimalPrecision);
  } catch (Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, "Error setting decimal precision for export: %1$s", e.what());
  }

  try {
    writer->WriteToCallback((Lib3MF::WriteCallback)lib3mf_write_callback,
                            (Lib3MF::SeekCallback)lib3mf_seek_callback, &output);
  } catch (Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
  }
  output.flush();
}
