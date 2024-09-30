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

#include "geometry/GeometryUtils.h"
#include "io/export.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALHybridPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include <cassert>
#include <ostream>
#include <utility>
#include <cstdint>
#include <memory>
#include <string>

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

#include "lib3mf_implicit.hpp"

#include <algorithm>

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#endif

static void export_3mf_error(std::string msg)
{
  LOG(message_group::Export_Error, std::move(msg));
}

/*
 * PolySet must be triangulated.
 */
static bool append_polyset(std::shared_ptr<const PolySet> ps, Lib3MF::PWrapper& wrapper, Lib3MF::PModel& model)
{
  try {
    auto mesh = model->AddMeshObject();
    if (!mesh) return false;
    mesh->SetName("OpenSCAD Model");

    auto vertexFunc = [&](const Vector3d& coords) -> bool {
      const auto f = coords.cast<float>();
      try {
        Lib3MF::sPosition v{f[0], f[1], f[2]};
        mesh->AddVertex(v);
      } catch (Lib3MF::ELib3MFException& e) {
        export_3mf_error(e.what());
        return false;
      }
      return true;
    };

    auto triangleFunc = [&](const IndexedFace& indices) -> bool {
      try {
        Lib3MF::sTriangle t{(Lib3MF_uint32)indices[0], (Lib3MF_uint32)indices[1], (Lib3MF_uint32)indices[2]};
        mesh->AddTriangle(t);
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

    for (const auto &v : out_ps->vertices) {
      if (!vertexFunc(v)) {
        export_3mf_error("Can't add vertex to 3MF model.");
        return false;
      }
    }

    for (const auto& poly : out_ps->indices) {
      if (!triangleFunc(poly)) {
        export_3mf_error("Can't add triangle to 3MF model.");
        return false;
      }
    }

    Lib3MF::PBuildItem builditem;
    try {
      model->AddBuildItem(mesh.get(), wrapper->GetIdentityTransform());
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
static bool append_nef(const CGAL_Nef_polyhedron& root_N, Lib3MF::PWrapper& wrapper, Lib3MF::PModel& model)
{
  if (!root_N.p3) {
    LOG(message_group::Export_Error, "Export failed, empty geometry.");
    return false;
  }

  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }

  if (std::shared_ptr<PolySet> ps = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3)) {
    return append_polyset(ps, wrapper, model);
  }
  export_3mf_error("Error converting NEF Polyhedron.");
  return false;
}
#endif

static bool append_3mf(const std::shared_ptr<const Geometry>& geom, Lib3MF::PWrapper& wrapper, Lib3MF::PModel& model)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      if (!append_3mf(item.second, wrapper, model)) return false;
    }
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    return append_nef(*N, wrapper, model);
  } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return append_polyset(hybrid->toPolySet(), wrapper, model);
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return append_polyset(mani->toPolySet(), wrapper, model);
#endif
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return append_polyset(PolySetUtils::tessellate_faces(*ps), wrapper, model);
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) {
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

void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  Lib3MF_uint32 interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro;
  Lib3MF::PWrapper wrapper;

  try {
    wrapper = Lib3MF::CWrapper::loadLibrary();
    wrapper->GetLibraryVersion(interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro);
    if (interfaceVersionMajor != LIB3MF_VERSION_MAJOR) {
      LOG(message_group::Error, "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d",
          interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro,
          LIB3MF_VERSION_MAJOR, LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO);
      return;
    }
  } catch (Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
    return;
  }

  if ((interfaceVersionMajor != LIB3MF_VERSION_MAJOR)) {
    LOG(message_group::Export_Error, "Invalid 3MF library major version %1$d.%2$d.%3$d, expected %4$d.%5$d.%6$d", interfaceVersionMajor, interfaceVersionMinor, interfaceVersionMicro, LIB3MF_VERSION_MAJOR, LIB3MF_VERSION_MINOR, LIB3MF_VERSION_MICRO);
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
  } catch (Lib3MF::ELib3MFException& e) {
    export_3mf_error("Can't get writer for 3MF model.");
    return;
  }

  try {
    writer->WriteToCallback((Lib3MF::WriteCallback)lib3mf_write_callback, (Lib3MF::SeekCallback)lib3mf_seek_callback, &output);
  } catch (Lib3MF::ELib3MFException& e) {
    LOG(message_group::Export_Error, e.what());
  }
  output.flush();
}
