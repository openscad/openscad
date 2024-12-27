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

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "geometry/Geometry.h"
#include "utils/printutils.h"
#include "utils/version_helper.h"
#include "core/AST.h"

#include <memory>
#include <string>

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
#include "geometry/cgal/cgalutils.h"
#endif

static std::unique_ptr<PolySet> import_3mf_error(PLib3MFModel *model = nullptr, PLib3MFModelResourceIterator *object_it = nullptr)
{
  if (model) {
    lib3mf_release(model);
  }
  if (object_it) {
    lib3mf_release(object_it);
  }

  return PolySet::createEmpty();
}

std::unique_ptr<Geometry> import_3mf(const std::string& filename, const Location& loc)
{
  DWORD interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  HRESULT result = lib3mf_getinterfaceversion(&interfaceVersionMajor, &interfaceVersionMinor, &interfaceVersionMicro);
  if (result != LIB3MF_OK) {
    LOG(message_group::Error, "Error reading 3MF library version");
    return PolySet::createEmpty();
  }

  if ((interfaceVersionMajor != NMR_APIVERSION_INTERFACE_MAJOR)) {
    LOG(message_group::Error, "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
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
    LOG(message_group::Warning, "Could not read file '%1$s', import() at line %2$d", filename.c_str(), loc.firstLine());
    return import_3mf_error(model);
  }

  PLib3MFModelResourceIterator *object_it;
  result = lib3mf_model_getmeshobjects(model, &object_it);
  if (result != LIB3MF_OK) {
    return import_3mf_error(model);
  }

  std::unique_ptr<PolySet> first_mesh;
  std::list<std::shared_ptr<PolySet>> meshes;
  unsigned int mesh_idx = 0;
  while (true) {
    int has_next;
    result = lib3mf_resourceiterator_movenext(object_it, &has_next);
    if (result != LIB3MF_OK) {
      return import_3mf_error(model, object_it);
    }
    if (!has_next) {
      break;
    }

    PLib3MFModelResource *object;
    result = lib3mf_resourceiterator_getcurrent(object_it, &object);
    if (result != LIB3MF_OK) {
      return import_3mf_error(model, object_it);
    }

    DWORD vertex_count;
    result = lib3mf_meshobject_getvertexcount(object, &vertex_count);
    if (result != LIB3MF_OK) {
      return import_3mf_error(model, object_it);
    }
    DWORD triangle_count;
    result = lib3mf_meshobject_gettrianglecount(object, &triangle_count);
    if (result != LIB3MF_OK) {
      return import_3mf_error(model, object_it);
    }

    PRINTDB("%s: mesh %d, vertex count: %lu, triangle count: %lu", filename.c_str() % mesh_idx % vertex_count % triangle_count);

    PolySetBuilder builder(0, triangle_count);
    for (DWORD idx = 0; idx < triangle_count; ++idx) {
      MODELMESHTRIANGLE triangle;
      if (lib3mf_meshobject_gettriangle(object, idx, &triangle) != LIB3MF_OK) {
        return import_3mf_error(model, object_it);
      }

      MODELMESHVERTEX vertex;

      builder.beginPolygon(3);
      for (auto m_nIndex : triangle.m_nIndices) {
        if (lib3mf_meshobject_getvertex(object, m_nIndex, &vertex) != LIB3MF_OK) {
          return import_3mf_error(model, object_it);
        }
        builder.addVertex(Vector3d(vertex.m_fPosition[0], vertex.m_fPosition[1], vertex.m_fPosition[2]));
      }
    }

    if (first_mesh) {
      meshes.push_back(builder.build());
    } else {
      first_mesh = builder.build();
    }
    mesh_idx++;
  }

  lib3mf_release(object_it);
  lib3mf_release(model);

  if (!first_mesh) {
    return PolySet::createEmpty();
  } else if (meshes.empty()) {
    return first_mesh;
  } else {
    std::unique_ptr<PolySet> p;
#ifdef ENABLE_CGAL
    Geometry::Geometries children;
    children.push_back(std::make_pair(std::shared_ptr<AbstractNode>(), std::shared_ptr<const Geometry>(std::move(first_mesh))));
    for (auto& meshe : meshes) {
      children.push_back(std::make_pair(std::shared_ptr<AbstractNode>(), std::shared_ptr<const Geometry>(meshe)));
    }
    if (auto ps = PolySetUtils::getGeometryAsPolySet(CGALUtils::applyUnion3D(children.begin(), children.end()))) {
      p = std::make_unique<PolySet>(*ps);
    } else {
      p = PolySet::createEmpty();
    }
#else
    p = PolySet::createEmpty();
#endif // ifdef ENABLE_CGAL
    return p;
  }
}
