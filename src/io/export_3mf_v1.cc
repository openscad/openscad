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

#include <Model/COM/NMR_DLLInterfaces.h>
#undef BOOL
using namespace NMR;

#include <algorithm>

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#endif

static void export_3mf_error(std::string msg, PLib3MFModel *& model)
{
  LOG(message_group::Export_Error, std::move(msg));
  if (model) {
    lib3mf_release(model);
    model = nullptr;
  }
}

/*
 * PolySet must be triangulated.
 */
static bool append_polyset(std::shared_ptr<const PolySet> ps, PLib3MFModelMeshObject *& model)
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

  auto vertexFunc = [&](const Vector3d& coords) -> bool {
    const auto f = coords.cast<float>();
    MODELMESHVERTEX v{f[0], f[1], f[2]};
    return lib3mf_meshobject_addvertex(mesh, &v, nullptr) == LIB3MF_OK;
  };


  auto triangleFunc = [&](const IndexedFace& indices) -> bool {
    MODELMESHTRIANGLE t{(DWORD)indices[0], (DWORD)indices[1], (DWORD)indices[2]};
    return lib3mf_meshobject_addtriangle(mesh, &t, nullptr) == LIB3MF_OK;
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

  PLib3MFModelBuildItem *builditem;
  if (lib3mf_model_addbuilditem(model, mesh, nullptr, &builditem) != LIB3MF_OK) {
    export_3mf_error("Can't add build item to 3MF model.", model);
    return false;
  }

  return true;
}

#ifdef ENABLE_CGAL
static bool append_nef(const CGAL_Nef_polyhedron& root_N, PLib3MFModelMeshObject *& model)
{
  if (!root_N.p3) {
    LOG(message_group::Export_Error, "Export failed, empty geometry.");
    return false;
  }

  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }


  if (std::shared_ptr<PolySet> ps = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3)) {
    return append_polyset(ps, model);
  }

  export_3mf_error("Error converting NEF Polyhedron.", model);
  return false;
}
#endif

static bool append_3mf(const std::shared_ptr<const Geometry>& geom, PLib3MFModelMeshObject *& model)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      if (!append_3mf(item.second, model)) return false;
    }
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    return append_nef(*N, model);
  } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return append_polyset(hybrid->toPolySet(), model);
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return append_polyset(mani->toPolySet(), model);
#endif
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return append_polyset(PolySetUtils::tessellate_faces(*ps), model);
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
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
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
    LOG(message_group::Export_Error, "Error writing 3MF model.");
  }


}
