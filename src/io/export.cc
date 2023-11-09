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

#include <string>
#include <vector>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

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

void exportFile(const shared_ptr<const Geometry>& root_geom, std::ostream& output, const ExportInfo& exportInfo)
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
  case FileFormat::LBRN:
    export_lbrn(root_geom, output, exportInfo);
    break;
  case FileFormat::PDF:
    export_pdf(root_geom, output, exportInfo);
    break;
  case FileFormat::NEFDBG:
    export_nefdbg(root_geom, output);
    break;
  case FileFormat::NEF3:
    export_nef3(root_geom, output);
    break;
  default:
    assert(false && "Unknown file format");
  }
}

bool exportFileByNameStdout(const shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  exportFile(root_geom, std::cout, exportInfo);
  return true;
}

bool exportFileByNameStream(const shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
  std::ios::openmode mode = std::ios::out | std::ios::trunc;
  if (exportInfo.format == FileFormat::_3MF || exportInfo.format == FileFormat::STL || exportInfo.format == FileFormat::PDF) {
    mode |= std::ios::binary;
  }
  std::ofstream fstream(exportInfo.name2open, mode);
  if (!fstream.is_open()) {
    LOG(_("Can't open file \"%1$s\" for export"), exportInfo.name2display);
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
      LOG(message_group::Error, _("\"%1$s\" write error. (Disk full?)"), exportInfo.name2display);
    }
    return !onerror;
  }
}

bool exportFileByName(const shared_ptr<const Geometry>& root_geom, const ExportInfo& exportInfo)
{
  bool exportResult = false;
  if (exportInfo.useStdOut) {
    exportResult = exportFileByNameStdout(root_geom, exportInfo);
  } else {
    exportResult = exportFileByNameStream(root_geom, exportInfo);
  }
  return exportResult;
}

namespace Export {

double normalize(double x) {
  return x == -0 ? 0 : x;
}

ExportMesh::Vertex vectorToVertex(const Vector3d& pt) {
  return {normalize(pt.x()), normalize(pt.y()), normalize(pt.z())};
}

ExportMesh::ExportMesh(const PolySet& ps)
{
  std::map<Vertex, int> vertexMap;
  std::vector<std::array<int, 3>> triangleIndices;

  for (const auto& pts : ps.polygons) {
    auto pos1 = vertexMap.emplace(std::make_pair(vectorToVertex(pts[0]), vertexMap.size()));
    auto pos2 = vertexMap.emplace(std::make_pair(vectorToVertex(pts[1]), vertexMap.size()));
    auto pos3 = vertexMap.emplace(std::make_pair(vectorToVertex(pts[2]), vertexMap.size()));
    triangleIndices.push_back({pos1.first->second, pos2.first->second, pos3.first->second});
  }

  std::vector<size_t> indexTranslationMap(vertexMap.size());
  vertices.reserve(vertexMap.size());

  size_t index = 0;
  for (const auto& e : vertexMap) {
    vertices.push_back(e.first);
    indexTranslationMap[e.second] = index++;
  }

  for (const auto& i : triangleIndices) {
    triangles.emplace_back(indexTranslationMap[i[0]], indexTranslationMap[i[1]], indexTranslationMap[i[2]]);
  }
  std::sort(triangles.begin(), triangles.end(), [](const Triangle& t1, const Triangle& t2) -> bool {
      return t1.key < t2.key;
    });
}

bool ExportMesh::foreach_vertex(const std::function<bool(const Vertex&)>& callback) const
{
  for (const auto& v : vertices) {
    if (!callback(v)) {
      return false;
    }
  }
  return true;
}

bool ExportMesh::foreach_indexed_triangle(const std::function<bool(const std::array<int, 3>&)>& callback) const
{
  for (const auto& t : triangles) {
    if (!callback(t.key)) {
      return false;
    }
  }
  return true;
}

bool ExportMesh::foreach_triangle(const std::function<bool(const std::array<std::array<double, 3>, 3>&)>& callback) const
{
  for (const auto& t : triangles) {
    auto& v0 = vertices[t.key[0]];
    auto& v1 = vertices[t.key[1]];
    auto& v2 = vertices[t.key[2]];
    if (!callback({ v0, v1, v2 })) {
      return false;
    }
  }
  return true;
}

} // namespace Export

ExportFileOptions::ExportFileOptions() 
{
  struct CommandExportOption ceo[] = { 
  	{ "lbrn", "colorIndex", "0",  "int"},
    { "lbrn", "minPower",   "40", "int"},
    { "lbrn", "maxPower",   "40", "int"},
    { "lbrn", "speed",      "8",  "int"},
    { "lbrn", "numPasses",  "1",  "int"},

	{ "pdf", "showScale",   std::to_string(1),   "bool"   },
	{ "pdf", "showScaleMg", std::to_string(1),   "bool"   },
	{ "pdf", "showGrid",    std::to_string(0),   "bool"   },
	{ "pdf", "gridSize",    std::to_string(10.), "float"  },
	{ "pdf", "showDsgnFN",  std::to_string(1),   "bool"   },
	{ "pdf", "Orientatio",  "Portrait",          "string" },
	{ "pdf", "paperSize",   "A4",                "string" }
  };
  
  for (int i = 0; i < sizeof(ceo) / sizeof(ceo[0]); i++) {
    add_option(ceo[i].format, ceo[i].option, ceo[i].value, ceo[i].type);
  }
}

bool ExportFileOptions::parse_command_export_option(const std::string& option)
{
  bool ret = false;
  CommandExportOption ceo;
  std::vector<std::string> parts;

  std::vector<std::pair<std::string::const_iterator, std::string::const_iterator> > tokens;
  boost::split(tokens, option.c_str(), boost::is_any_of(".="));
  for(auto beg=tokens.begin(); beg!=tokens.end();++beg)
  {
    parts.push_back(std::string(beg->first, beg->second));
  }

  if (parts.size() == 3) {
    ceo.format = std::string(parts.at(0));
    ceo.option = std::string(parts.at(1));
    ceo.value  = std::string(parts.at(2));
  
  	update_option(ceo.format, ceo.option, ceo.value);
  	
  	ret = true;
  }

  return ret;
}

bool ExportFileOptions::add_option(const std::string& format, const std::string& option, const std::string& value, const std::string& type)
{
  bool exists = option_exists(format, option);
  CommandExportOption ceo;
  	
  if (exists == false) {
    ceo.format = format;
    ceo.option = option;
    ceo.value  = value;
    ceo.type   = type;

    commandExportOptions.push_back(ceo);
  }
  
  return true;
}

bool ExportFileOptions::update_option(const std::string& format, const std::string& option, const std::string& value)
{
  bool exists = option_exists(format, option);
  CommandExportOption ceo;
  bool ret = false;
  	
  if (exists == true) {
    int idx = get_option_index(format, option);
    if (idx != -1) {
      ceo = get_option(format, option);
	  if (is_value_valid(format, option, value) == true) {
        commandExportOptions[idx].value = value;
        ret = true;
      } else {
        LOG("Invalid file export option value: %1$s - %2$s should of type %3$s", format, option, ceo.type);
      }
    }
  } else {
    LOG("Invalid file export option: %1$s - %2$s", format, option);
  }
  
  return ret;
}

bool ExportFileOptions::option_exists(const std::string& format, const std::string& option)
{
  int pos = get_option_index(format, option);
  bool ret = true;
  if (pos == -1)
    ret = false;
		
  return ret;
}

int ExportFileOptions::get_option_index(const std::string& format, const std::string& option)
{
  int idx = -1;
  int pos = -1;
	
  for(CommandExportOption cmdOpt : commandExportOptions) {
    pos++;
    if (strcmp(cmdOpt.format.c_str(), format.c_str()) == 0 && strcmp(cmdOpt.option.c_str(), option.c_str()) == 0) {
      idx = pos;
    }
  }

  return idx;
}

CommandExportOption ExportFileOptions::get_option(const std::string& format, const std::string& option)
{
  CommandExportOption ceo;
  ceo.format = "";
  ceo.option = "";
  ceo.value = "";
  ceo.type = "";
	
  for(CommandExportOption cmdOpt : commandExportOptions) {
    if (strcmp(cmdOpt.format.c_str(), format.c_str()) == 0 && strcmp(cmdOpt.option.c_str(), option.c_str()) == 0) {
      ceo.format = cmdOpt.format;
      ceo.option = cmdOpt.option;
      ceo.value = cmdOpt.value;
      ceo.type = cmdOpt.type;
    }
  }
  
  return ceo;
}


std::string ExportFileOptions::get_option_value(const std::string& format, const std::string& option)
{
  if (option_exists(format, option)) {
    CommandExportOption ceo = get_option(format, option);
    return ceo.value;
  }
  
  return NULL;
}


std::vector<CommandExportOption> ExportFileOptions::get_options(const std::string& format)
{
  std::vector<CommandExportOption> options;

  for(CommandExportOption cmdOpt : commandExportOptions) {
  	if (strcmp(cmdOpt.format.c_str(), format.c_str()) == 0) {
   	  CommandExportOption ceo;
   	  
   	  ceo.format = cmdOpt.format;
   	  ceo.option = cmdOpt.option;
   	  ceo.value = cmdOpt.value;
   	  
   	  options.push_back(ceo);
  	}
  }
  
  return options;	
}		

bool ExportFileOptions::is_number(const std::string& str)
{
  return !str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end();
}

bool ExportFileOptions::is_float(const std::string& str)
{
  std::string::difference_type n = std::count(str.begin(), str.end(), '.');
  if (n != 1) return false;
  
  return !str.empty() && std::find_if(str.begin(), 
    str.end(), [](unsigned char c) { return !std::isdigit(c) && c != '.'; }) == str.end();
}

bool ExportFileOptions::is_bool(const std::string& str)
{
  std::string lowercaseStr = str;
  std::transform(
    lowercaseStr.begin(),
    lowercaseStr.end(),
    lowercaseStr.begin(),
    [](unsigned char c) {
      return std::tolower(c);
    });
  
  return (lowercaseStr == "true" || lowercaseStr == "false" || lowercaseStr == "0" || lowercaseStr == "1");
}

bool ExportFileOptions::is_string(const std::string& str)
{
  return true;
}

bool ExportFileOptions::is_value_valid(const std::string& format, const std::string& option, const std::string& value)
{
  bool ret = false;
  int idx = get_option_index(format, option);
  
  if (idx != -1) {
  	CommandExportOption ceo = get_option(format, option);
  	
    if (ceo.type == "int") {
      if (is_number(value) == true)
        ret = true;
    
    } else if (ceo.type == "float") {
      if (is_float(value) == true)
        ret = true;
    
    } else if (ceo.type == "bool") {
      if (is_bool(value) == true)
        ret = true;
    
    } else if (ceo.type == "string") {
      ret = true;
    
    }
  }
  
  return ret;
}

