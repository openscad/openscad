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

#include "io/export.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"
#include "geometry/Geometry.h"

#include <algorithm>
#include <functional>
#include <cassert>
#include <map>
#include <cstdint>
#include <memory>
#include <cstddef>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

namespace {

struct Containers {
  std::unordered_map<std::string, FileFormatInfo> identifierToInfo;
  std::map<FileFormat, FileFormatInfo> fileFormatToInfo;
};

void add_item(Containers& containers, const FileFormatInfo& info) {
  containers.identifierToInfo[info.identifier] = info;
  containers.fileFormatToInfo[info.format] = info;
}

Containers &containers() {
  static std::unique_ptr<Containers> containers = [](){
    auto containers = std::make_unique<Containers>();

    add_item(*containers, {FileFormat::ASCII_STL, "asciistl", "stl", "STL (ascii)"});
    add_item(*containers, {FileFormat::BINARY_STL, "binstl", "stl", "STL (binary)"});
    add_item(*containers, {FileFormat::OBJ, "obj", "obj", "OBJ"});
    add_item(*containers, {FileFormat::OFF, "off", "off", "OFF"});
    add_item(*containers, {FileFormat::WRL, "wrl", "wrl", "VRML"});
    add_item(*containers, {FileFormat::AMF, "amf", "amf", "AMF"});
    add_item(*containers, {FileFormat::_3MF, "3mf", "3mf", "3MF"});
    add_item(*containers, {FileFormat::DXF, "dxf", "dxf", "DXF"});
    add_item(*containers, {FileFormat::SVG, "svg", "svg", "SVG"});
    add_item(*containers, {FileFormat::NEFDBG, "nefdbg", "nefdbg", "nefdbg"});
    add_item(*containers, {FileFormat::NEF3, "nef3", "nef3", "nef3"});
    add_item(*containers, {FileFormat::CSG, "csg", "csg", "CSG"});
    add_item(*containers, {FileFormat::PARAM, "param", "param", "param"});
    add_item(*containers, {FileFormat::AST, "ast", "ast", "AST"});
    add_item(*containers, {FileFormat::TERM, "term", "term", "term"});
    add_item(*containers, {FileFormat::ECHO, "echo", "echo", "echo"});
    add_item(*containers, {FileFormat::PNG, "png", "png", "PNG"});
    add_item(*containers, {FileFormat::PDF, "pdf", "pdf", "PDF"});
    add_item(*containers, {FileFormat::POV, "pov", "pov", "POV"});

    // Alias
    containers->identifierToInfo["stl"] = containers->identifierToInfo["asciistl"];
  
    return std::move(containers);
  }();
  return *containers;
}

std::unordered_map<std::string, FileFormatInfo> &identifierToInfo() {
  static auto identifierToInfo = std::make_unique<std::unordered_map<std::string, FileFormatInfo>>();
  return *identifierToInfo;
}

std::unordered_map<FileFormat, FileFormatInfo> &fileFormatToInfo() {
  static auto fileFormatToInfo = std::make_unique<std::unordered_map<FileFormat, FileFormatInfo>>();
  return *fileFormatToInfo;
}

}  // namespace

namespace fileformat {

std::vector<FileFormat> all()
{
  std::vector<FileFormat> allFileFormats;
  for (const auto& item : containers().fileFormatToInfo) {
    allFileFormats.push_back(item.first);
  }
  return allFileFormats;
}

std::vector<FileFormat> all2D()
{
  std::vector<FileFormat> all2DFormats;
  for (const auto& item : containers().fileFormatToInfo) {
    if (is2D(item.first)) {
      all2DFormats.push_back(item.first);
    }
  }
  return all2DFormats;
}

std::vector<FileFormat> all3D()
{
  std::vector<FileFormat> all3DFormats;
  for (const auto& item : containers().fileFormatToInfo) {
    if (is3D(item.first)) {
      all3DFormats.push_back(item.first);
    }
  }
  return all3DFormats;
}

const FileFormatInfo& info(FileFormat fileFormat)
{
  return containers().fileFormatToInfo[fileFormat];
}

bool fromIdentifier(const std::string& identifier, FileFormat& format)
{
  auto it = containers().identifierToInfo.find(identifier);
  if (it == containers().identifierToInfo.end()) return false;
  format = it->second.format;
  return true;
}

const std::string& toSuffix(FileFormat format)
{
  return containers().fileFormatToInfo[format].suffix;
}

bool canPreview(FileFormat format) {
  return (format == FileFormat::AST ||
          format == FileFormat::CSG ||
          format == FileFormat::PARAM ||
          format == FileFormat::ECHO ||
          format == FileFormat::TERM ||
          format == FileFormat::PNG);
}

bool is3D(FileFormat format) {
return format == FileFormat::ASCII_STL ||
  format == FileFormat::BINARY_STL ||
  format == FileFormat::OBJ ||
  format == FileFormat::OFF ||
  format == FileFormat::WRL ||
  format == FileFormat::AMF ||
  format == FileFormat::_3MF ||
  format == FileFormat::NEFDBG ||
  format == FileFormat::NEF3 ||
  format == FileFormat::POV;
}

bool is2D(FileFormat format) {
  return format == FileFormat::DXF ||
    format == FileFormat::SVG ||
    format == FileFormat::PDF;
}

}  // namespace FileFormat

void exportFile(const std::shared_ptr<const Geometry>& root_geom, std::ostream& output, const ExportInfo& exportInfo)
{
  switch (exportInfo.format) {
  case FileFormat::ASCII_STL:
    export_stl(root_geom, output, false);
    break;
  case FileFormat::BINARY_STL:
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
  case FileFormat::POV:
    export_pov(root_geom, output, exportInfo);
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

bool exportFileStdOut(const std::shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  exportFile(root_geom, std::cout, exportInfo);
  return true;
}

bool exportFileByName(const std::shared_ptr<const Geometry>& root_geom, const std::string& filename, const ExportInfo& exportInfo)
{
  std::ios::openmode mode = std::ios::out | std::ios::trunc;
  if (exportInfo.format == FileFormat::_3MF || exportInfo.format == FileFormat::BINARY_STL || exportInfo.format == FileFormat::PDF) {
    mode |= std::ios::binary;
  }
  const std::filesystem::path path(filename);
  std::ofstream fstream(path, mode);
  if (!fstream.is_open()) {
    LOG(_("Can't open file \"%1$s\" for export"), filename);
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
      LOG(message_group::Error, _("\"%1$s\" write error. (Disk full?)"), filename);
    }
    return !onerror;
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
  out->color_indices = ps.color_indices;
  out->colors = ps.colors;

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
  if (ps.color_indices.empty()) {
    std::sort(out->indices.begin(), out->indices.end());
  } else {
    struct ColoredFace {
      IndexedFace face;
      int32_t color_index;
    };
    std::vector<ColoredFace> faces;
    faces.reserve(ps.indices.size());
    for (size_t i = 0, n = ps.indices.size(); i < n; i++) {
      faces.push_back({out->indices[i], out->color_indices[i]});
    }
    std::sort(faces.begin(), faces.end(), [](const ColoredFace& a, const ColoredFace& b) {
      return a.face < b.face;
    });
    for (size_t i = 0, n = faces.size(); i < n; i++) {
      auto & face = faces[i];
      out->indices[i] = face.face;
      out->color_indices[i] = face.color_index;
    }
  }
  return out;
}
