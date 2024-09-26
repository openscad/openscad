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
#include "lib3mf_implicit.hpp"

#include <utility>
#include <memory>
#include <vector>

namespace {
  std::unique_ptr<PolySet> getAsPolySet(const Lib3MF::PMeshObject& object) {
    try {
      if (!object) return nullptr;

      Lib3MF_uint64 vertex_count = object->GetVertexCount();
      Lib3MF_uint64 triangle_count = object->GetTriangleCount();
      if (!vertex_count || !triangle_count) return nullptr;

      PolySetBuilder builder(0,triangle_count);
      for (Lib3MF_uint64 idx = 0; idx < triangle_count; ++idx) {
        Lib3MF::sTriangle triangle = object->GetTriangle(idx);

        builder.beginPolygon(3);

        for (unsigned int idx : triangle.m_Indices) {
          Lib3MF::sPosition vertex = object->GetVertex(idx);
          builder.addVertex({vertex.m_Coordinates[0], vertex.m_Coordinates[1], vertex.m_Coordinates[2]});
        }
      }
      return builder.build();
    } catch (const Lib3MF::ELib3MFException& e) {
      LOG(message_group::Error, e.what());
      return nullptr;
    }
  }
}

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
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
  }

  const OpenSCAD::library_version_number header_version{LIB3MF_VERSION_MAJOR, LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO};
  const OpenSCAD::library_version_number runtime_version{interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro};
  return OpenSCAD::get_version_string(header_version, runtime_version);
}

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#endif

std::unique_ptr<Geometry> import_3mf(const std::string& filename, const Location& loc)
{
  Lib3MF::PWrapper wrapper;

  try {
    wrapper = Lib3MF::CWrapper::loadLibrary();
    Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
    wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
    if (interfaceVersionMajor != LIB3MF_VERSION_MAJOR) {
      LOG(message_group::Error, "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
          interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro,
          LIB3MF_VERSION_MAJOR, LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO);
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
    LOG(message_group::Export_Error, "Could create 3MF reader, import() at line %1$d: %2$s", loc.firstLine(), e.what());
    return PolySet::createEmpty();
  }

  try {
    reader->ReadFromFile(filename);
  } catch (const Lib3MF::ELib3MFException& e) {
    LOG(message_group::Warning, "Could not read file '%1$s', import() at line %2$d: %3$s", filename.c_str(), loc.firstLine(), e.what());
    return PolySet::createEmpty();
  }

  Lib3MF::PMeshObjectIterator object_it = model->GetMeshObjects();
  if (!object_it) {
    return PolySet::createEmpty();
  }

  std::unique_ptr<PolySet> first_mesh;
  std::vector<std::unique_ptr<PolySet>> meshes;
  unsigned int mesh_idx = 0;
  while (object_it->MoveNext()) {
      auto ps = getAsPolySet(object_it->GetCurrentMeshObject());
      // FIXME: Should we just return an empty object for all these cases, or is it valid to return existing geometry?
      if (!ps) return PolySet::createEmpty();
      PRINTDB("%s: mesh %d, vertex count: %lu, triangle count: %lu", filename.c_str() % mesh_idx % ps->vertices.size() % ps->indices.size());
      meshes.push_back(std::move(ps));
      mesh_idx++;
  }

  if (meshes.size() == 1) {
    return std::move(meshes.front());
  } else {
    std::unique_ptr<PolySet> p;
#ifdef ENABLE_CGAL
    Geometry::Geometries children;
    for (auto& ps : meshes) {
      children.emplace_back(std::shared_ptr<const AbstractNode>(), std::shared_ptr<const PolySet>(std::move(ps)));
    }
    if (auto ps = PolySetUtils::getGeometryAsPolySet(CGALUtils::applyUnion3D(children.begin(), children.end()))) {
      // FIXME: unnecessary copy
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
