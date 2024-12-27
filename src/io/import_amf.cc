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

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"
#include "core/AST.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#endif

#include <utility>
#include <memory>
#include <sys/types.h>
#include <cstddef>
#include <map>
#include <cassert>
#include <string>
#include <vector>
#include <libxml/xmlreader.h>
#include <filesystem>
#include <boost/lexical_cast.hpp>

static const std::string text_node("#text");
static const std::string object("/amf/object");
static const std::string coordinates("/amf/object/mesh/vertices/vertex/coordinates");
static const std::string coordinates_x = coordinates + "/x";
static const std::string coordinates_y = coordinates + "/y";
static const std::string coordinates_z = coordinates + "/z";
static const std::string triangle("/amf/object/mesh/volume/triangle");
static const std::string triangle_v1 = triangle + "/v1";
static const std::string triangle_v2 = triangle + "/v2";
static const std::string triangle_v3 = triangle + "/v3";

class AmfImporter
{
private:
  std::string xpath; // element nesting stack

  using cb_func = void (*)(AmfImporter *, const xmlChar *);

  std::unique_ptr<PolySetBuilder> builder;
  std::vector<std::unique_ptr<PolySet>> polySets;

  double x{0}, y{0}, z{0};
  int idx_v1{0}, idx_v2{0}, idx_v3{0};
  std::vector<Eigen::Vector3d> vertex_list;

  std::map<const std::string, cb_func> funcs;
  std::map<const std::string, cb_func> start_funcs;
  std::map<const std::string, cb_func> end_funcs;

  static void set_x(AmfImporter *importer, const xmlChar *value);
  static void set_y(AmfImporter *importer, const xmlChar *value);
  static void set_z(AmfImporter *importer, const xmlChar *value);
  static void set_v1(AmfImporter *importer, const xmlChar *value);
  static void set_v2(AmfImporter *importer, const xmlChar *value);
  static void set_v3(AmfImporter *importer, const xmlChar *value);
  static void start_object(AmfImporter *importer, const xmlChar *value);
  static void end_object(AmfImporter *importer, const xmlChar *value);
  static void end_triangle(AmfImporter *importer, const xmlChar *vlue);
  static void end_vertex(AmfImporter *importer, const xmlChar *value);

  int streamFile(const char *filename);
  void processNode(xmlTextReaderPtr reader);

protected:
  const Location& loc;

public:
  AmfImporter(const Location& loc);
  virtual ~AmfImporter() = default;
  std::unique_ptr<PolySet> read(const std::string& filename);

  virtual xmlTextReaderPtr createXmlReader(const char *filename);
};

AmfImporter::AmfImporter(const Location& loc) : loc(loc)
{
}

void AmfImporter::set_x(AmfImporter *importer, const xmlChar *value)
{
  importer->x = boost::lexical_cast<double>(std::string((const char *)value));
}

void AmfImporter::set_y(AmfImporter *importer, const xmlChar *value)
{
  importer->y = boost::lexical_cast<double>(std::string((const char *)value));
}

void AmfImporter::set_z(AmfImporter *importer, const xmlChar *value)
{
  importer->z = boost::lexical_cast<double>(std::string((const char *)value));
}

void AmfImporter::set_v1(AmfImporter *importer, const xmlChar *value)
{
  importer->idx_v1 = boost::lexical_cast<int>(std::string((const char *)value));
}

void AmfImporter::set_v2(AmfImporter *importer, const xmlChar *value)
{
  importer->idx_v2 = boost::lexical_cast<int>(std::string((const char *)value));
}

void AmfImporter::set_v3(AmfImporter *importer, const xmlChar *value)
{
  importer->idx_v3 = boost::lexical_cast<int>(std::string((const char *)value));
}

void AmfImporter::start_object(AmfImporter *importer, const xmlChar *)
{
  importer->builder = std::make_unique<PolySetBuilder>(0,0);
}

void AmfImporter::end_object(AmfImporter *importer, const xmlChar *)
{
  PRINTDB("AMF: add object %d", importer->polySets.size());
  importer->polySets.push_back(importer->builder->build());
  importer->vertex_list.clear();
  importer->builder.reset(nullptr);
}

void AmfImporter::end_vertex(AmfImporter *importer, const xmlChar *)
{
  PRINTDB("AMF: add vertex %d - (%.2f, %.2f, %.2f)", importer->vertex_list.size() % importer->x % importer->y % importer->z);
  importer->vertex_list.emplace_back(importer->x, importer->y, importer->z);
}

void AmfImporter::end_triangle(AmfImporter *importer, const xmlChar *)
{
  int idx[3]= {importer->idx_v1,importer->idx_v2,importer->idx_v3};
  PRINTDB("AMF: add triangle %d - (%.2f, %.2f, %.2f)", importer->vertex_list.size() % idx[0] % idx[1] % idx[2]);

  std::vector<Eigen::Vector3d>& v = importer->vertex_list;

  importer->builder->beginPolygon(3);
  for(auto i : idx) // TODO set vertex array first
	  importer->builder->addVertex(Vector3d(v[i].x(), v[i].y(), v[i].z()));
}

void AmfImporter::processNode(xmlTextReaderPtr reader)
{
  const char *name = reinterpret_cast<const char *>(xmlTextReaderName(reader));
  if (name == nullptr) name = reinterpret_cast<const char *>(xmlStrdup(BAD_CAST "--"));
  xmlChar *value = xmlTextReaderValue(reader);
  int node_type = xmlTextReaderNodeType(reader);
  switch (node_type) {
  case XML_READER_TYPE_ELEMENT:
  {
    xpath += '/';
    xpath += name;
    cb_func startFunc = start_funcs[xpath];
    if (startFunc) {
      PRINTDB("AMF: start %s", xpath);
      startFunc(this, nullptr);
    }
  }
  break;
  case XML_READER_TYPE_END_ELEMENT:
  {
    cb_func endFunc = end_funcs[xpath];
    if (endFunc) {
      PRINTDB("AMF: end   %s", xpath);
      endFunc(this, value);
    }
    size_t pos = xpath.find_last_of('/');
    if (pos != std::string::npos) xpath.erase(pos);
  }
  break;
  case XML_READER_TYPE_TEXT:
  {
    cb_func textFunc = funcs[xpath];
    if (textFunc) {
      PRINTDB("AMF: text  %s - '%s'", xpath % value);
      textFunc(this, value);
    }
  }
  break;
  }

  xmlFree(value);
  xmlFree((void *) (name));
}

xmlTextReaderPtr AmfImporter::createXmlReader(const char *filename)
{
  return xmlReaderForFile(filename, nullptr, XML_PARSE_NOENT | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
}

int AmfImporter::streamFile(const char *filename)
{
  int ret;

  xmlTextReaderPtr reader = createXmlReader(filename);

  if (reader == nullptr) {
    LOG(message_group::Warning, "Can't open import file '%1$s', import() at line %2$d", filename, this->loc.firstLine());
    return 1;
  }

  try {
    xmlTextReaderSetParserProp(reader, XML_PARSER_SUBST_ENTITIES, 1);
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
      processNode(reader);
      ret = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
  } catch (boost::bad_lexical_cast&) {
    ret = -1;
  }
  if (ret != 0) {
    LOG(message_group::Warning, "Failed to parse file '%1$s', import() at line %2$d", filename, this->loc.firstLine());
  }
  return ret;
}

std::unique_ptr<PolySet> AmfImporter::read(const std::string& filename)
{
  funcs[coordinates_x] = set_x;
  funcs[coordinates_y] = set_y;
  funcs[coordinates_z] = set_z;
  funcs[triangle_v1] = set_v1;
  funcs[triangle_v2] = set_v2;
  funcs[triangle_v3] = set_v3;
  start_funcs[object] = start_object;
  end_funcs[coordinates] = end_vertex;
  end_funcs[triangle] = end_triangle;
  end_funcs[object] = end_object;
  streamFile(filename.c_str());
  vertex_list.clear();

  if (polySets.empty()) {
    return PolySet::createEmpty();
  }
  if (polySets.size() == 1) {
    return std::move(polySets[0]);
  }
  if (polySets.size() > 1) {
    Geometry::Geometries children;
    for (auto& polySet : polySets) {
      children.push_back(std::make_pair(std::shared_ptr<AbstractNode>(), std::move(polySet)));
    }

#ifdef ENABLE_CGAL
    std::unique_ptr<const Geometry> geom = CGALUtils::applyUnion3D(children.begin(), children.end());
    if (auto ps = PolySetUtils::getGeometryAsPolySet(std::move(geom))) {
      // FIXME: Unnecessary copy
      return std::make_unique<PolySet>(*ps);
    } else
#endif // ENABLE_CGAL
      LOG(message_group::Error, "Error importing multi-object AMF file '%1$s', import() at line %2$d", filename, this->loc.firstLine());
  }
  return PolySet::createEmpty();
}

#ifdef ENABLE_LIBZIP

#include <zip.h>

class AmfImporterZIP : public AmfImporter
{
private:
  struct zip *archive {nullptr};
  struct zip_file *zipfile {nullptr};

  static int read_callback(void *context, char *buffer, int len);
  static int close_callback(void *context);

public:
  AmfImporterZIP(const Location& loc);

  xmlTextReaderPtr createXmlReader(const char *filename) override;
};

AmfImporterZIP::AmfImporterZIP(const Location& loc) : AmfImporter(loc)
{
}

int AmfImporterZIP::read_callback(void *context, char *buffer, int len)
{
  auto *importer = (AmfImporterZIP *)context;
  return zip_fread(importer->zipfile, buffer, len);
}

int AmfImporterZIP::close_callback(void *context)
{
  auto *importer = (AmfImporterZIP *)context;
  return zip_fclose(importer->zipfile);
}

xmlTextReaderPtr AmfImporterZIP::createXmlReader(const char *filepath)
{
  archive = zip_open(filepath, 0, nullptr);
  if (archive) {
    // Separate the filename without using filesystem::path because that gives wide result on Windows TM
    const char *last_slash = strrchr(filepath, '/');
    const char *last_bslash = strrchr(filepath, '\\');
    if (last_bslash > last_slash) last_slash = last_bslash;
    const char *filename = last_slash ? last_slash + 1 : filepath;
    zipfile = zip_fopen(archive, filename, ZIP_FL_NODIR);
    if (zipfile == nullptr) {
      LOG(message_group::Warning, "Can't read file '%1$s' from zipped AMF '%2$s', import() at line %3$d", filename, filepath, this->loc.firstLine());
    }
    if ((zipfile == nullptr) && (zip_get_num_entries(archive, 0) == 1)) {
      LOG(message_group::Warning, "Trying to read single entry '%1$s'", zip_get_name(archive, 0, 0));
      zipfile = zip_fopen_index(archive, 0, 0);
    }
    if (zipfile) {
      return xmlReaderForIO(read_callback, close_callback, this, filename, nullptr,
                            XML_PARSE_NOENT | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    } else {
      zip_close(archive);
      zipfile = nullptr;
      return nullptr;
    }
  } else {
    return AmfImporter::createXmlReader(filepath);
  }
}

std::unique_ptr<PolySet> import_amf(const std::string& filename, const Location& loc) {
  LOG(message_group::Deprecated, "AMF import is deprecated. Please use 3MF instead.");
  AmfImporterZIP importer(loc);
  return importer.read(filename);
}

#else

std::unique_ptr<PolySet> import_amf(const std::string& filename, const Location& loc) {
  LOG(message_group::Deprecated, "AMF import is deprecated. Please use 3MF instead.");
  AmfImporter importer(loc);
  return importer.read(filename);
}

#endif // ifdef ENABLE_LIBZIP
