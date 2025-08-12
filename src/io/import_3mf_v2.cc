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
#include "io/import.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <lib3mf_implicit.hpp>

#include "core/AST.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "glview/RenderSettings.h"
#include "utils/printutils.h"
#include "utils/version_helper.h"
#include "io/lib3mf_utils.h"

namespace {

struct MeshObject {
  const Lib3MF::PMeshObject obj;
  const Matrix4d transform;
};

struct ModelMetadata {
  std::string title;
  std::string designer;
  std::string description;
  std::string copyright;
  std::string licenseterms;
  std::string rating;
  std::string creationdate;
  std::string modificationdate;
  std::string application;
};

using MeshObjectList = std::list<std::unique_ptr<MeshObject>>;

std::string get_object_type_name(const Lib3MF::eObjectType objecttype)
{
  switch (objecttype) {
  case Lib3MF::eObjectType::Other:        return "Other";
  case Lib3MF::eObjectType::Model:        return "Model";
  case Lib3MF::eObjectType::Support:      return "Support";
  case Lib3MF::eObjectType::SolidSupport: return "Solid Support";
  default:                                return "<Unknown>";
  }
}

Matrix4d get_matrix(Lib3MF::sTransform& transform)
{
  Matrix4d tm;
  // clang-format off
  tm << transform.m_Fields[0][0], transform.m_Fields[1][0], transform.m_Fields[2][0], transform.m_Fields[3][0],
        transform.m_Fields[0][1], transform.m_Fields[1][1], transform.m_Fields[2][1], transform.m_Fields[3][1],
        transform.m_Fields[0][2], transform.m_Fields[1][2], transform.m_Fields[2][2], transform.m_Fields[3][2],
        0,                        0,                        0,                        1;
  // clang-format on
  return tm;
}

std::string collect_mesh_objects(const Lib3MF::PModel& model, MeshObjectList& object_list,
                                 const Lib3MF::PObject& object, const Matrix4d& m, const Location& loc,
                                 int level = 1)
{
  const bool is_mesh_object = object->IsMeshObject();
  const bool is_components_object = object->IsComponentsObject();
  const auto objecttype = object->GetType();
  const auto partnumber = object->GetPartNumber();
  const auto name = object->GetName();
  bool hasuuid = false;
  const auto uuid = object->GetUUID(hasuuid);

#if 0
  // Validation disabled for now, this crashes in some cases. One example
  // is the MicroscopeMainBody.3mf from the blog post
  // https://axotron.se/blog/microscope-adapter-for-mobile-phone-camera/
  const auto is_valid_object = object->IsValid();
  if (!is_valid_object) {
    LOG(message_group::Warning, "Object '%1$s' with UUID '%2$s' is not valid, import() at line %3$d", name, uuid, loc.firstLine());
  }
#endif

  if (is_mesh_object) {
    const auto meshobject = model->GetMeshObjectByID(object->GetUniqueResourceID());
    PRINTDB("%smesh type = %s, number = '%s', name = '%s' (%s)",
            boost::io::group(std::setw(2 * level), "") % get_object_type_name(objecttype) % partnumber %
              name % (hasuuid ? uuid : "<no uuid>"));
    object_list.push_back(std::make_unique<MeshObject>(MeshObject{meshobject, m}));
    return "";
  }
  if (is_components_object) {
    const auto componentsobject = model->GetComponentsObjectByID(object->GetUniqueResourceID());
    const int componentcount = componentsobject->GetComponentCount();
    PRINTDB("%sobject (%d components) type = %s, number = '%s', name = '%s' (%s)",
            boost::io::group(std::setw(2 * level), "") % componentcount %
              get_object_type_name(objecttype) % partnumber % name % (hasuuid ? uuid : "<no uuid>"));

    for (int idx = 0; idx < componentcount; ++idx) {
      const auto component = componentsobject->GetComponent(idx);
      const bool has_transform = component->HasTransform();
      Lib3MF::sTransform transform{.m_Fields = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 1}}};
      if (has_transform) {
        transform = component->GetTransform();
      }
      const auto componentobject = component->GetObjectResource();
      const Matrix4d cm = get_matrix(transform);
      if (has_transform) {
        PRINTDB("%scomponent transform matrix\n%s", boost::io::group(std::setw(2 * level), "") % cm);
      }
      auto errmsg = collect_mesh_objects(model, object_list, componentobject, cm * m, loc, level + 1);
      if (!errmsg.empty()) {
        return errmsg;
      }
    }
    return "";
  }
  return "Unhandled object type, expected one of: mesh, component";
}

// lib3mf_propertyhandler_getcolor states:
// (#00000000) means no property or a different kind of property!
Color4f get_color(const Lib3MF::PColorGroup& colorgroup, const Lib3MF_uint32 propertyid)
{
  const auto color = colorgroup->GetColor(propertyid);
  if (color.m_Red == 0 && color.m_Green == 0 && color.m_Blue == 0 && color.m_Alpha == 0) {
    return {};  // invalid color
  }
  Color4f c{color.m_Red, color.m_Green, color.m_Blue, color.m_Alpha};
  return c;
}

Color4f get_triangle_color(const Lib3MF::PModel& model,
                           const Lib3MF::sTriangleProperties triangle_properties)
{
  const auto colorgroup = model->GetColorGroupByID(triangle_properties.m_ResourceID);
  const Color4f col0 = get_color(colorgroup, triangle_properties.m_PropertyIDs[0]);
  const Color4f col1 = get_color(colorgroup, triangle_properties.m_PropertyIDs[1]);
  const Color4f col2 = get_color(colorgroup, triangle_properties.m_PropertyIDs[2]);
  if (col0.isValid() && col1.isValid() && col2.isValid()) {
    return {std::clamp((col0.r() + col1.r() + col2.r()) / 3, 0.0f, 1.0f),
            std::clamp((col0.g() + col1.g() + col2.g()) / 3, 0.0f, 1.0f),
            std::clamp((col0.b() + col1.b() + col2.b()) / 3, 0.0f, 1.0f),
            std::clamp((col0.a() + col1.a() + col2.a()) / 3, 0.0f, 1.0f)};
  }
  return {};
}

Color4f get_triangle_color_from_basematerial(const Lib3MF::PModel& model,
                                             const Lib3MF::sTriangleProperties triangle_properties)
{
  const auto basematerialgroup = model->GetBaseMaterialGroupByID(triangle_properties.m_ResourceID);
  const auto displaycolor = basematerialgroup->GetDisplayColor(triangle_properties.m_PropertyIDs[0]);
  Color4f col{displaycolor.m_Red, displaycolor.m_Green, displaycolor.m_Blue, 255};
  return col;
}

Color4f get_triangle_color(const Lib3MF::PModel& model, const Lib3MF::PMeshObject& object, int idx)
{
  Lib3MF::sTriangleProperties triangle_properties;
  object->GetTriangleProperties(idx, triangle_properties);
  if (triangle_properties.m_ResourceID == 0) {
    return {};
  }

  const auto propertytype = model->GetPropertyTypeByID(triangle_properties.m_ResourceID);
  switch (propertytype) {
  case Lib3MF::ePropertyType::BaseMaterial:
    return get_triangle_color_from_basematerial(model, triangle_properties);
  case Lib3MF::ePropertyType::Colors: return get_triangle_color(model, triangle_properties);
  default:                            return {};
  }
}

std::string import_3mf_mesh(const std::string& filename, unsigned int mesh_idx,
                            const Lib3MF::PModel& model, const std::unique_ptr<MeshObject>& mo,
                            std::unique_ptr<PolySet>& ps)
{
  const auto object = mo->obj;
  const auto vertex_count = object->GetVertexCount();
  const auto triangle_count = object->GetTriangleCount();
  if (!vertex_count || !triangle_count) {
    return "Empty mesh";
  }

  const auto object_type = get_object_type_name(object->GetType());

  PRINTDB("%s: mesh %d, type: %s, vertex count: %lu, triangle count: %lu",
          filename.c_str() % mesh_idx % object_type % vertex_count % triangle_count);

  ps->vertices.reserve(vertex_count);
  ps->indices.reserve(triangle_count);
  ps->color_indices.reserve(triangle_count);

  std::vector<Lib3MF::sPosition> all_vertices;
  object->GetVertices(all_vertices);
  for (const auto& vertex : all_vertices) {
    const Vector4d v = mo->transform * Vector4d(vertex.m_Coordinates[0], vertex.m_Coordinates[1],
                                                vertex.m_Coordinates[2], 1);
    ps->vertices.push_back(v.head(3));
  }

  std::unordered_map<Color4f, int32_t> color_indices;
  for (Lib3MF_uint32 idx = 0; idx < triangle_count; ++idx) {
    const auto triangle = object->GetTriangle(idx);
    ps->indices.emplace_back();
    for (const auto& idx : triangle.m_Indices) {
      ps->indices.back().push_back(idx);
    }

    const Color4f col = get_triangle_color(model, object, idx);
    if (col.isValid()) {
      const auto it = color_indices.find(col);
      int32_t cidx;
      if (it == color_indices.end()) {
        cidx = ps->colors.size();
        ps->colors.push_back(col);
        color_indices[col] = cidx;
      } else {
        cidx = it->second;
      }
      ps->color_indices.push_back(cidx);
    } else {
      ps->color_indices.push_back(-1);
    }
  }
  if (ps->colors.empty()) {
    ps->color_indices.clear();
  }
  ps->setTriangular(true);

  return "";
}

std::string read_metadata(const Lib3MF::PModel& model)
{
  const auto metadatagroup = model->GetMetaDataGroup();
  const auto metadatacount = metadatagroup->GetMetaDataCount();

  ModelMetadata mmd;
  for (Lib3MF_uint32 idx = 0; idx < metadatacount; ++idx) {
    const auto metadata = metadatagroup->GetMetaData(idx);
    const auto key = metadata->GetKey();
    const auto value = metadata->GetValue();
    PRINTDB("METADATA[%d]: %s = '%s'", idx % key % value);

    const std::string& name = key;
    if (name == "Title") {
      mmd.title = value;
    } else if (name == "Designer") {
      mmd.designer = value;
    } else if (name == "Description") {
      mmd.description = value;
    } else if (name == "Copyright") {
      mmd.copyright = value;
    } else if (name == "LicenseTerms") {
      mmd.licenseterms = value;
    } else if (name == "Rating") {
      mmd.rating = value;
    } else if (name == "CreationDate") {
      mmd.creationdate = value;
    } else if (name == "ModificationDate") {
      mmd.modificationdate = value;
    } else if (name == "Application") {
      mmd.application = value;
    }
  }

  if (!mmd.title.empty()) {
    LOG("Reading 3MF with title '%1$s'", mmd.title);
  }

  return "";
}

}  // namespace

std::string get_lib3mf_version()
{
  Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  Lib3MF::PWrapper wrapper;

  try {
    wrapper = Lib3MF::CWrapper::loadLibrary();
    wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
  }

  const OpenSCAD::library_version_number header_version{LIB3MF_VERSION_MAJOR, LIB3MF_VERSION_MINOR,
                                                        LIB3MF_VERSION_MICRO};
  const OpenSCAD::library_version_number runtime_version{interfaceVersionMajor, interfaceVersionMinor,
                                                         interfaceVersionMicro};
  return OpenSCAD::get_version_string(header_version, runtime_version);
}

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif

std::unique_ptr<PolySet> import_3mf(const std::string& filename, const Location& loc)
{
  Lib3MF::PWrapper wrapper;

  try {
    wrapper = Lib3MF::CWrapper::loadLibrary();
    Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
    wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
    if (interfaceVersionMajor != LIB3MF_VERSION_MAJOR) {
      LOG(message_group::Error,
          "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
          interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro, LIB3MF_VERSION_MAJOR,
          LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO);
      return PolySet::createEmpty();
    }
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
    return PolySet::createEmpty();
  }

  Lib3MF::PModel model;
  try {
    model = wrapper->CreateModel();
    if (!model) {
      LOG(message_group::Error, "Could not create model");
      return PolySet::createEmpty();
    }
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
    return PolySet::createEmpty();
  }

  Lib3MF::PReader reader;
  try {
    reader = model->QueryReader("3mf");
    if (!reader) {
      LOG(message_group::Error, "Could not create 3MF reader");
      return PolySet::createEmpty();
    }
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, "Could create 3MF reader, import() at line %1$d: %2$s",
        loc.firstLine(), e.what());
    return PolySet::createEmpty();
  }

  try {
    reader->ReadFromFile(filename);
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Warning, "Could not read file '%1$s', import() at line %2$d: %3$s",
        filename.c_str(), loc.firstLine(), e.what());
    return PolySet::createEmpty();
  }

  read_metadata(model);

  try {
    Lib3MF::PBuildItemIterator builditem_it = model->GetBuildItems();
    if (!builditem_it) {
      LOG(message_group::Warning, "Could not retrieve build items, import() at line %2$d",
          filename.c_str(), loc.firstLine());
      return PolySet::createEmpty();
    }

    std::list<std::unique_ptr<PolySet>> meshes;
    while (builditem_it->MoveNext()) {
      const auto builditem = builditem_it->GetCurrent();
      const auto builditemhandle = builditem->GetObjectResourceID();
      const auto partnumber = builditem->GetPartNumber();
      bool hasuuid = false;
      const auto uuid = builditem->GetUUID(hasuuid);
      Lib3MF::sTransform transform{.m_Fields = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}}};
      if (builditem->HasObjectTransform()) {
        transform = builditem->GetObjectTransform();
      }
      const auto object = builditem->GetObjectResource();
      const Matrix4d m = get_matrix(transform);

      PRINTDB("build item %d, part number = '%s' (%s)",
              builditemhandle % partnumber % (hasuuid ? uuid : "<no uuid>"));
      if (builditem->HasObjectTransform()) {
        PRINTDB("build item transform matrix\n%s", m);
      }

      MeshObjectList object_list;
      std::string errmsg = collect_mesh_objects(model, object_list, object, m, loc);
      if (!errmsg.empty()) {
        LOG(message_group::Warning, "Error collecting meshes, import() at line %2$d", filename.c_str(),
            loc.firstLine());
        return PolySet::createEmpty();
      }

      for (const auto& mo : object_list) {
        auto ps = PolySet::createEmpty();
        std::string errmsg = import_3mf_mesh(filename, meshes.size(), model, mo, ps);
        if (errmsg.empty()) {
          if (ps->isEmpty()) {
            continue;
          }
        } else {
          LOG(message_group::Warning, "%1$s, import() at line %2$d", errmsg, loc.firstLine());
          return PolySet::createEmpty();
        }

        meshes.push_back(std::move(ps));
      }
    }

    if (meshes.empty()) {
      return PolySet::createEmpty();
    } else if (meshes.size() == 1) {
      return std::move(meshes.front());
    } else {
      std::unique_ptr<PolySet> p;
      Geometry::Geometries children;
      while (!meshes.empty()) {
        children.emplace_back(std::shared_ptr<AbstractNode>(),
                              std::shared_ptr<const Geometry>(std::move(meshes.front())));
        meshes.pop_front();
      }
#ifdef ENABLE_MANIFOLD
      if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
        if (auto ps = PolySetUtils::getGeometryAsPolySet(
              ManifoldUtils::applyOperator3DManifold(children, OpenSCADOperator::UNION))) {
          p = std::make_unique<PolySet>(*ps);
        } else {
          p = PolySet::createEmpty();
        }
      } else
#endif  // ifdef ENABLE_MANIFOLD
      {
#ifdef ENABLE_CGAL
        if (auto ps = PolySetUtils::getGeometryAsPolySet(
              CGALUtils::applyUnion3D(children.begin(), children.end()))) {
          p = std::make_unique<PolySet>(*ps);
        } else {
          p = PolySet::createEmpty();
        }
#else
        p = PolySet::createEmpty();
#endif  // ifdef ENABLE_CGAL
      }
      return p;
    }
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Error, e.what());
    return nullptr;
  }
}
