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

#include <cstddef>
#include <functional>
#include <cstdint>
#include <memory>
#include <string>
#include <iomanip>
#include <algorithm>

#include <Model/COM/NMR_DLLInterfaces.h>

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "utils/printutils.h"
#include "utils/version_helper.h"
#include "core/AST.h"
#include "glview/RenderSettings.h"
#include "io/lib3mf_utils.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif

#undef BOOL
using namespace NMR;

std::string get_lib3mf_version()
{
  DWORD major, minor, micro;
  NMR::lib3mf_getinterfaceversion(&major, &minor, &micro);

  const OpenSCAD::library_version_number header_version{
    NMR_APIVERSION_INTERFACE_MAJOR, NMR_APIVERSION_INTERFACE_MINOR, NMR_APIVERSION_INTERFACE_MICRO};
  const OpenSCAD::library_version_number runtime_version{major, minor, micro};
  return OpenSCAD::get_version_string(header_version, runtime_version);
}

namespace {

struct MeshObject {
  PLib3MFModelObjectResource *obj;
  Matrix4d transform;
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

// lib3mf_propertyhandler_getcolor states:
// (#00000000) means no property or a different kind of property!
Color4f get_color(const MODELMESHCOLOR_SRGB& color)
{
  if (color.m_Red == 0 && color.m_Green == 0 && color.m_Blue == 0 && color.m_Alpha == 0) {
    return {};  // invalid color
  }
  Color4f c{color.m_Red, color.m_Green, color.m_Blue, color.m_Alpha};
  return c;
}

Color4f get_color(const MODELMESH_TRIANGLECOLOR_SRGB& color, int idx)
{
  return get_color(color.m_Colors[idx]);
}

Matrix4d get_matrix(MODELTRANSFORM& transform)
{
  Matrix4d tm;
  // clang-format off
  tm << transform.m_fFields[0][0], transform.m_fFields[0][1], transform.m_fFields[0][2], transform.m_fFields[0][3],
        transform.m_fFields[1][0], transform.m_fFields[1][1], transform.m_fFields[1][2], transform.m_fFields[1][3],
        transform.m_fFields[2][0], transform.m_fFields[2][1], transform.m_fFields[2][2], transform.m_fFields[2][3],
        0,                         0,                         0,                         1;
  // clang-format on
  return tm;
}

std::string get_object_type_name(DWORD objecttype)
{
  switch (objecttype) {
  case MODELOBJECTTYPE_OTHER:        return "Other";
  case MODELOBJECTTYPE_MODEL:        return "Model";
  case MODELOBJECTTYPE_SUPPORT:      return "Support";
  case MODELOBJECTTYPE_SOLIDSUPPORT: return "Solid Support";
  case MODELOBJECTTYPE_SURFACE:      return "Surface";
  default:                           return "<Unknown>";
  }
}

std::unique_ptr<PolySet> import_3mf_error(PLib3MFModel *model = nullptr, const std::string& errmsg = "",
                                          PLib3MFModelBuildItemIterator *it = nullptr)
{
  if (!errmsg.empty()) {
    LOG(message_group::Error, "%1$s", errmsg);
  }

  if (model) {
    lib3mf_release(model);
  }
  if (it) {
    lib3mf_release(it);
  }

  return PolySet::createEmpty();
}

std::string collect_mesh_objects(MeshObjectList& object_list, PLib3MFModelObjectResource *object,
                                 const Matrix4d& m, const Location& loc, int level = 1)
{
  BOOL is_mesh_object = false;
  if (lib3mf_object_ismeshobject(object, &is_mesh_object) != LIB3MF_OK) {
    return "Could not check for mesh object type";
  }
  BOOL is_components_object = false;
  if (lib3mf_object_iscomponentsobject(object, &is_components_object) != LIB3MF_OK) {
    return "Could not check for component object type";
  }
  DWORD objecttype;
  if (lib3mf_object_gettype(object, &objecttype) != LIB3MF_OK) {
    return "Could not read object type";
  }
  char number[4096] = {
    0,
  };
  ULONG numberlen;
  if (lib3mf_object_getpartnumberutf8(object, &number[0], sizeof(number), &numberlen) != LIB3MF_OK) {
    return "Could not read part number of object";
  }
  char name[4096] = {
    0,
  };
  ULONG namelen;
  if (lib3mf_object_getnameutf8(object, &name[0], sizeof(name), &namelen) != LIB3MF_OK) {
    return "Could not read name of object";
  }
  BOOL hasuuid = false;
  char uuid[40] = {
    0,
  };
  if (lib3mf_object_getuuidutf8(object, &hasuuid, &uuid[0]) != LIB3MF_OK) {
    return "Could not read UUID of object";
  }

  BOOL is_valid_object = false;
  if (lib3mf_object_isvalidobject(object, &is_valid_object) != LIB3MF_OK) {
    return "Could not check object validity";
  }
  if (!is_valid_object) {
    LOG(message_group::Warning, "Object '%1$s' with UUID '%2$s' is not valid, import() at line %3$d",
        name, uuid, loc.firstLine());
  }

  if (is_mesh_object) {
    PRINTDB("%smesh type = %s, number = '%s', name = '%s' (%s)",
            boost::io::group(std::setw(2 * level), "") % get_object_type_name(objecttype) % number %
              name % (hasuuid ? uuid : "<no uuid>"));
    object_list.push_back(std::make_unique<MeshObject>(MeshObject{object, m}));
    return "";
  }
  if (is_components_object) {
    DWORD componentcount;
    if (lib3mf_componentsobject_getcomponentcount(object, &componentcount) != LIB3MF_OK) {
      return "Could not get object component count";
    }
    PRINTDB("%sobject (%d components) type = %s, number = '%s', name = '%s' (%s)",
            boost::io::group(std::setw(2 * level), "") % componentcount %
              get_object_type_name(objecttype) % number % name % (hasuuid ? uuid : "<no uuid>"));
    for (DWORD idx = 0; idx < componentcount; ++idx) {
      PLib3MFModelComponent *component = nullptr;
      if (lib3mf_componentsobject_getcomponent(object, idx, &component) != LIB3MF_OK) {
        return "Could not get object component";
      }
      BOOL has_transform = false;
      if (lib3mf_component_hastransform(component, &has_transform) != LIB3MF_OK) {
        return "Could not check for component transform";
      }
      MODELTRANSFORM transform{.m_fFields = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
      if (has_transform && lib3mf_component_gettransform(component, &transform) != LIB3MF_OK) {
        return "Could not read transform of component";
      }
      PLib3MFModelObjectResource *componentobject = nullptr;
      if (lib3mf_component_getobjectresource(component, &componentobject) != LIB3MF_OK) {
        return "Could not read object resource of component";
      }
      const Matrix4d cm = get_matrix(transform);
      if (has_transform) {
        PRINTDB("%scomponent transform matrix\n%s", boost::io::group(std::setw(2 * level), "") % cm);
      }
      auto errmsg = collect_mesh_objects(object_list, componentobject, cm * m, loc, level + 1);
      if (!errmsg.empty()) {
        return errmsg;
      }
    }
    return "";
  }
  return "Unhandled object type, expected one of: mesh, component";
}

Color4f get_triangle_color_from_basematerial(PLib3MFModel *model,
                                             PLib3MFPropertyHandler *propertyhandler, int idx)
{
  DWORD materialIndex = 0;
  DWORD materialGroupID = 0;

  if (lib3mf_propertyhandler_getbasematerial(propertyhandler, idx, &materialGroupID, &materialIndex) !=
      LIB3MF_OK) {
    return {};
  }
  if (materialGroupID == 0) {
    return {};
  }

  PLib3MFModelBaseMaterial *basematerial;
  if (lib3mf_model_getbasematerialbyid(model, materialGroupID, &basematerial) != LIB3MF_OK) {
    return {};
  }

  BYTE r = 0, g = 0, b = 0, a = 0;
  if (lib3mf_basematerial_getdisplaycolor(basematerial, materialIndex, &r, &g, &b, &a) != LIB3MF_OK) {
    return {};
  }

  Color4f col{r, g, b, 255};
  return col;
}

Color4f get_triangle_color(PLib3MFPropertyHandler *propertyhandler, int idx)
{
  MODELMESH_TRIANGLECOLOR_SRGB color = {.m_Colors = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}};
  if (lib3mf_propertyhandler_getcolor(propertyhandler, idx, &color) == LIB3MF_OK) {
    const Color4f col0 = get_color(color, 0);
    const Color4f col1 = get_color(color, 1);
    const Color4f col2 = get_color(color, 2);
    if (col0.isValid() && col1.isValid() && col2.isValid()) {
      return {std::clamp((col0.r() + col1.r() + col2.r()) / 3, 0.0f, 1.0f),
              std::clamp((col0.g() + col1.g() + col2.g()) / 3, 0.0f, 1.0f),
              std::clamp((col0.b() + col1.b() + col2.b()) / 3, 0.0f, 1.0f),
              std::clamp((col0.a() + col1.a() + col2.a()) / 3, 0.0f, 1.0f)};
    }
  }
  return {};
}

Color4f get_triangle_color(PLib3MFModel *model, PLib3MFPropertyHandler *propertyhandler, int idx)
{
  eModelPropertyType propertytype = MODELPROPERTYTYPE_NONE;
  lib3mf_propertyhandler_getpropertytype(propertyhandler, idx, &propertytype);
  if (propertytype == MODELPROPERTYTYPE_BASEMATERIALS) {
    return get_triangle_color_from_basematerial(model, propertyhandler, idx);
  }
  if (propertytype == MODELPROPERTYTYPE_COLOR) {
    return get_triangle_color(propertyhandler, idx);
  }
  return {};
}

std::string import_3mf_mesh(const std::string& filename, unsigned int mesh_idx, PLib3MFModel *model,
                            std::unique_ptr<MeshObject>& mo, std::unique_ptr<PolySet>& ps)
{
  DWORD vertex_count = 0;
  if (lib3mf_meshobject_getvertexcount(mo->obj, &vertex_count) != LIB3MF_OK) {
    return "Could not read vertex count";
  }
  DWORD triangle_count = 0;
  if (lib3mf_meshobject_gettrianglecount(mo->obj, &triangle_count) != LIB3MF_OK) {
    return "Could not read triangle count";
  }
  DWORD object_type = 0;
  if (lib3mf_object_gettype(mo->obj, &object_type) != LIB3MF_OK) {
    return "Could not read object type";
  }
  PRINTDB(
    "%s: mesh %d, type: %s, vertex count: %lu, triangle count: %lu",
    filename.c_str() % mesh_idx % get_object_type_name(object_type) % vertex_count % triangle_count);

  PLib3MFPropertyHandler *propertyhandler = nullptr;
  if (lib3mf_meshobject_createpropertyhandler(mo->obj, &propertyhandler) != LIB3MF_OK) {
    return "Could not create property handler";
  }

  ps->vertices.reserve(vertex_count);
  ps->indices.reserve(triangle_count);
  ps->color_indices.reserve(triangle_count);

  for (DWORD idx = 0; idx < vertex_count; ++idx) {
    MODELMESHVERTEX vertex;
    if (lib3mf_meshobject_getvertex(mo->obj, idx, &vertex) != LIB3MF_OK) {
      return "Could not read vertex from object";
    }
    const Vector4d v =
      mo->transform * Vector4d(vertex.m_fPosition[0], vertex.m_fPosition[1], vertex.m_fPosition[2], 1);
    ps->vertices.push_back(v.head(3));
  }

  std::unordered_map<Color4f, int32_t> color_indices;
  for (DWORD idx = 0; idx < triangle_count; ++idx) {
    MODELMESHTRIANGLE triangle;
    if (lib3mf_meshobject_gettriangle(mo->obj, idx, &triangle) != LIB3MF_OK) {
      return "Could not read triangle from object";
    }
    ps->indices.emplace_back();
    for (DWORD vertex_idx : triangle.m_nIndices) {
      ps->indices.back().push_back(vertex_idx);
    }

    const Color4f col = get_triangle_color(model, propertyhandler, idx);
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

  lib3mf_release(propertyhandler);
  return "";
}

std::string read_metadata(PLib3MFModel *model)
{
  DWORD metadatacount;
  if (lib3mf_model_getmetadatacount(model, &metadatacount) != LIB3MF_OK) {
    return "Could not retrieve metadata";
  }

  ModelMetadata mmd;
  for (DWORD idx = 0; idx < metadatacount; ++idx) {
    char key[4096] = {
      0,
    };
    ULONG keylen = 0;
    if (lib3mf_model_getmetadatakeyutf8(model, idx, &key[0], sizeof(key), &keylen) != LIB3MF_OK) {
      return "Could not retrieve metadata key";
    }
    char value[4096] = {
      0,
    };
    ULONG valuelen = 0;
    if (lib3mf_model_getmetadatavalueutf8(model, idx, &value[0], sizeof(value), &valuelen) !=
        LIB3MF_OK) {
      return "Could not retrieve metadata value";
    }
    PRINTDB("METADATA[%d]: %s = '%s'", idx % key % value);

    const std::string name = key;
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

std::unique_ptr<PolySet> import_3mf(const std::string& filename, const Location& loc)
{
  DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  HRESULT result =
    lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
  if (result != LIB3MF_OK) {
    LOG(message_group::Error, "Error reading 3MF library version");
    return PolySet::createEmpty();
  }

  if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
    LOG(message_group::Error,
        "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
        interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro,
        NMR_APIVERSION_INTERFACE_MAJOR, NMR_APIVERSION_INTERFACE_MINOR, NMR_APIVERSION_INTERFACE_MICRO);
    return PolySet::createEmpty();
  }

  PLib3MFModel *model;
  result = lib3mf_createmodel(&model);
  if (result != LIB3MF_OK) {
    LOG(message_group::Error, "Could not create model: %1$08lx", result);
    return import_3mf_error();
  }

  PLib3MFModelReader *reader;
  result = lib3mf_model_queryreader(model, "3mf", &reader);
  if (result != LIB3MF_OK) {
    LOG(message_group::Error, "Could not create 3MF reader: %1$08lx", result);
    return import_3mf_error(model);
  }

  result = lib3mf_reader_readfromfileutf8(reader, filename.c_str());
  lib3mf_release(reader);
  if (result != LIB3MF_OK) {
    LOG(message_group::Warning, "Could not read file '%1$s', import() at line %2$d", filename.c_str(),
        loc.firstLine());
    return import_3mf_error(model);
  }

  read_metadata(model);

  PLib3MFModelBuildItemIterator *builditem_it = nullptr;
  if (lib3mf_model_getbuilditems(model, &builditem_it) != LIB3MF_OK) {
    return import_3mf_error(model, "Could not retrieve build items");
  }

  std::list<std::unique_ptr<PolySet>> meshes;
  while (true) {
    BOOL has_next = false;
    if (lib3mf_builditemiterator_movenext(builditem_it, &has_next) != LIB3MF_OK) {
      return import_3mf_error(model, "Error iterating over build items", builditem_it);
    }
    if (!has_next) {
      break;
    }

    PLib3MFModelBuildItem *builditem = nullptr;
    if (lib3mf_builditemiterator_getcurrent(builditem_it, &builditem) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not read build item", builditem_it);
    }
    DWORD builditemhandle;
    if (lib3mf_builditem_gethandle(builditem, &builditemhandle) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not get handle of build item", builditem_it);
    }
    char partnumber[4096] = {
      0,
    };
    ULONG partnumberlen = 0;
    if (lib3mf_builditem_getpartnumberutf8(builditem, &partnumber[0], sizeof(partnumber),
                                           &partnumberlen) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not get part number of build item", builditem_it);
    }
    BOOL hasuuid = false;
    char uuid[40] = {
      0,
    };
    if (lib3mf_builditem_getuuidutf8(builditem, &hasuuid, &uuid[0]) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not get uuid of build item", builditem_it);
    }
    PLib3MFModelObjectResource *object = nullptr;
    if (lib3mf_builditem_getobjectresource(builditem, &object) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not read object resource of build item", builditem_it);
    }

    BOOL has_transform = false;
    if (lib3mf_builditem_hasobjecttransform(builditem, &has_transform) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not check for build item transform", builditem_it);
    }
    MODELTRANSFORM transform{.m_fFields = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
    if (has_transform && lib3mf_builditem_getobjecttransform(builditem, &transform) != LIB3MF_OK) {
      return import_3mf_error(model, "Could not read build item transform", builditem_it);
    }
    const Matrix4d m = get_matrix(transform);

    PRINTDB("build item %d, part number = '%s' (%s)",
            builditemhandle % partnumber % (hasuuid ? uuid : "<no uuid>"));
    if (has_transform) {
      PRINTDB("build item transform matrix\n%s", m);
    }

    MeshObjectList object_list;
    std::string errmsg = collect_mesh_objects(object_list, object, m, loc);
    if (!errmsg.empty()) {
      return import_3mf_error(model, "Error collecting meshes: " + errmsg, builditem_it);
    }

    for (auto& mo : object_list) {
      auto ps = PolySet::createEmpty();
      std::string errmsg = import_3mf_mesh(filename, meshes.size(), model, mo, ps);
      if (errmsg.empty()) {
        if (ps->isEmpty()) {
          continue;
        }
      } else {
        return import_3mf_error(model, errmsg, builditem_it);
      }

      meshes.push_back(std::move(ps));
    }
  }

  lib3mf_release(builditem_it);
  lib3mf_release(model);

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
}
