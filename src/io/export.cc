/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
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
#include "PolySet.h"
#include "printutils.h"
#include "Geometry.h"

#include <fstream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

bool canPreview(const FileFormat format) {
  return (format == FileFormat::AST ||
          format == FileFormat::CSG ||
          format == FileFormat::PARAM ||
          format == FileFormat::ECHO ||
          format == FileFormat::TERM ||
          format == FileFormat::PNG);
}

bool is3D(const FileFormat format) {
return format == FileFormat::ASCIISTL ||
  format == FileFormat::STL ||
  format == FileFormat::OBJ ||
  format == FileFormat::OFF ||
  format == FileFormat::WRL ||
  format == FileFormat::AMF ||
  format == FileFormat::_3MF ||
  format == FileFormat::NEFDBG ||
  format == FileFormat::NEF3;
}

bool is2D(const FileFormat format) {
  return format == FileFormat::DXF ||
    format == FileFormat::SVG ||
    format == FileFormat::PDF;
}

void exportFile(const std::shared_ptr<const Geometry>& root_geom, std::ostream& output, const ExportInfo& exportInfo)
{
  switch (exportInfo.format) {
  case FileFormat::ASCIISTL:
    export_stl(root_geom, output, false);
    break;
  case FileFormat::STL:
    export_stl(root_geom, output, true);
    break;
  case FileFormat::OBJ:
    export_obj(root_geom, output);
    break;
  case FileFormat::OFF:
    export_off(root_geom, output);
    break;
  case FileFormat::WRL:
    export_wrl(root_geom, output);
    break;
  case FileFormat::AMF:
    export_amf(root_geom, output);
    break;
  case FileFormat::_3MF:
    export_3mf(root_geom, output);
    break;
  case FileFormat::DXF:
    export_dxf(root_geom, output);
    break;
  case FileFormat::SVG:
    export_svg(root_geom, output);
    break;
  case FileFormat::PDF:
    export_pdf(root_geom, output, exportInfo);
    break;
#ifdef ENABLE_CGAL
  case FileFormat::NEFDBG:
    export_nefdbg(root_geom, output);
    break;
  case FileFormat::NEF3:
    export_nef3(root_geom, output);
    break;
#endif
  default:
    assert(false && "Unknown file format");
  }
}

bool exportFileByNameStdout(const std::shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  exportFile(root_geom, std::cout, exportInfo);
  return true;
}

bool exportFileByNameStream(const std::shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
  std::ios::openmode mode = std::ios::out | std::ios::trunc;
  if (exportInfo.format == FileFormat::_3MF || exportInfo.format == FileFormat::STL || exportInfo.format == FileFormat::PDF) {
    mode |= std::ios::binary;
  }
  std::ofstream fstream(exportInfo.fileName, mode);
  if (!fstream.is_open()) {
    LOG(_("Can't open file \"%1$s\" for export"), exportInfo.displayName);
    return false;
  } else {
    bool onerror = false;
    fstream.exceptions(std::ios::badbit | std::ios::failbit);
    try {
      exportFile(root_geom, fstream, exportInfo);
    } catch (std::ios::failure&) {
      onerror = true;
    }
    try { // make sure file closed - resources released
      fstream.close();
    } catch (std::ios::failure&) {
      onerror = true;
    }
    if (onerror) {
      LOG(message_group::Error, _("\"%1$s\" write error. (Disk full?)"), exportInfo.displayName);
    }
    return !onerror;
  }
}

bool exportFileByName(const std::shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
  if (exportInfo.useStdOut) {
    return exportFileByNameStdout(root_geom, exportInfo);
  } else {
    return exportFileByNameStream(root_geom, exportInfo);
  }
}

namespace {

double remove_negative_zero(double x) {
  return x == -0 ? 0 : x;
}

Vector3d remove_negative_zero(const Vector3d& pt) {
  return {
    remove_negative_zero(pt[0]), 
    remove_negative_zero(pt[1]), 
    remove_negative_zero(pt[2]),
  };
}

#if EIGEN_VERSION_AT_LEAST(3,4,0)
// Eigen 3.4.0 added begin()/end()
struct LexographicLess {
  template<class T>
  bool operator()(T const& lhs, T const& rhs) const {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::less{});
  }
};
#else
struct LexographicLess {
  template<class T>
  bool operator()(T const& lhs, T const& rhs) const {
    return std::lexicographical_compare(lhs.data(), lhs.data() + lhs.size(), rhs.data(), rhs.data() + rhs.size(), std::less{});
  }
};
#endif

} // namespace 

std::unique_ptr<PolySet> createSortedPolySet(const PolySet& ps)
{
  auto out = std::make_unique<PolySet>(ps.getDimension(), ps.convexValue());
  out->setTriangular(ps.isTriangular());
  out->setConvexity(ps.getConvexity());

  std::map<Vector3d, int, LexographicLess> vertexMap;

  for (const auto& poly : ps.indices) {
    IndexedFace face;
    for (const auto idx : poly) {
      auto pos = vertexMap.emplace(remove_negative_zero(ps.vertices[idx]), vertexMap.size());
      face.push_back(pos.first->second);
    }
    out->indices.push_back(face);
  }

  std::vector<int> indexTranslationMap(vertexMap.size());
  out->vertices.reserve(vertexMap.size());

  for (const auto& [v,i] : vertexMap) {
    indexTranslationMap[i] = out->vertices.size();
    out->vertices.push_back(v);
  }

  for (auto& poly : out->indices) {
    IndexedFace polygon;
    for (const auto idx : poly) {
      polygon.push_back(indexTranslationMap[idx]);
    }
    std::rotate(polygon.begin(), std::min_element(polygon.begin(), polygon.end()), polygon.end());
    poly = polygon;
  }
  std::sort(out->indices.begin(), out->indices.end());

  return out;
}
