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

#include "importnode.h"

#include "polyset.h"
#include "printutils.h"
#include "AST.h"

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

#include <sys/types.h>
#include <map>
#include <fstream>
#include <assert.h>
#include <libxml/xmlreader.h>
#include <boost/filesystem.hpp>
#include "boost-utils.h"

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

class AmfImporter {
private:
	std::string xpath;   // element nesting stack

	typedef void (*cb_func)(AmfImporter *, const xmlChar *);

	PolySet *polySet;
	std::vector<PolySet *> polySets;
	
	double x, y, z;
	int idx_v1, idx_v2, idx_v3;
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
	const Location &loc;
	
public:
	AmfImporter(const Location &loc);
	virtual ~AmfImporter();
	PolySet *read(const std::string filename);
	
	virtual xmlTextReaderPtr createXmlReader(const char *filename);
};

AmfImporter::AmfImporter(const Location &loc) : polySet(nullptr), x(0), y(0), z(0), idx_v1(0), idx_v2(0), idx_v3(0), loc(loc)
{
}

AmfImporter::~AmfImporter()
{
	delete polySet;
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
	importer->polySet = new PolySet(3);
}

void AmfImporter::end_object(AmfImporter *importer, const xmlChar *)
{
	PRINTDB("AMF: add object %d", importer->polySets.size());
	importer->polySets.push_back(importer->polySet);
	importer->vertex_list.clear();
	importer->polySet = nullptr;
}

void AmfImporter::end_vertex(AmfImporter *importer, const xmlChar *)
{
	PRINTDB("AMF: add vertex %d - (%.2f, %.2f, %.2f)", importer->vertex_list.size() % importer->x % importer->y % importer->z);
	importer->vertex_list.push_back(Eigen::Vector3d(importer->x, importer->y, importer->z));
}

void AmfImporter::end_triangle(AmfImporter *importer, const xmlChar *)
{
	int idx_v1 = importer->idx_v1;
	int idx_v2 = importer->idx_v2;
	int idx_v3 = importer->idx_v3;
	PRINTDB("AMF: add triangle %d - (%.2f, %.2f, %.2f)", importer->vertex_list.size() % idx_v1 % idx_v2 % idx_v3);

	std::vector<Eigen::Vector3d> &v = importer->vertex_list;
	
	importer->polySet->append_poly();
	importer->polySet->append_vertex(v[idx_v1].x(), v[idx_v1].y(), v[idx_v1].z());
	importer->polySet->append_vertex(v[idx_v2].x(), v[idx_v2].y(), v[idx_v2].z());
	importer->polySet->append_vertex(v[idx_v3].x(), v[idx_v3].y(), v[idx_v3].z());
}

void AmfImporter::processNode(xmlTextReaderPtr reader)
{
	const char *name = reinterpret_cast<const char *> (xmlTextReaderName(reader));
	if (name == nullptr)
		name = reinterpret_cast<const char *> (xmlStrdup(BAD_CAST "--"));
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
        if(pos != std::string::npos)
            xpath.erase(pos);
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
		LOG(message_group::Warning,Location::NONE,"","Can't open import file '%1$s', import() at line %2$d",filename,this->loc.firstLine());
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
	} catch (boost::bad_lexical_cast &) {
		ret = -1;
	}
	if (ret != 0) {
		LOG(message_group::Warning,Location::NONE,"","Failed to parse file '%1$s', import() at line %2$d",filename,this->loc.firstLine());
	}
	return ret;
}

PolySet * AmfImporter::read(const std::string filename)
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

	PolySet *p = nullptr;
#ifdef ENABLE_CGAL
	if (polySets.size() == 1) {
		p = polySets[0];
	} if (polySets.size() > 1) {
		Geometry::Geometries children;
		for (std::vector<PolySet *>::iterator it = polySets.begin(); it != polySets.end(); ++it) {
			children.push_back(std::make_pair((const AbstractNode*)nullptr,  shared_ptr<const Geometry>(*it)));
		}
		CGAL_Nef_polyhedron *N = CGALUtils::applyUnion3D(children.begin(), children.end());
		PolySet *result = new PolySet(3);
		if (CGALUtils::createPolySetFromNefPolyhedron3(*N->p3, *result)) {
			delete result;
			p = new PolySet(3);
			LOG(message_group::Error,Location::NONE,"","Error importing multi-object AMF file '%1$s', import() at line %2$d",filename,this->loc.firstLine());
		} else {
			p = result;
		}
		delete N;
	}
#endif
	if (!p) {
		p = new PolySet(3);
	}
	polySets.clear();
	return p;
}

#ifdef ENABLE_LIBZIP

#include <zip.h>

class AmfImporterZIP : public AmfImporter
{
private:
	struct zip *archive;
	struct zip_file *zipfile;

	static int read_callback(void *context, char *buffer, int len);
	static int close_callback(void *context);

public:
	AmfImporterZIP(const Location &loc);
	~AmfImporterZIP();

	xmlTextReaderPtr createXmlReader(const char *filename) override;
};

AmfImporterZIP::AmfImporterZIP(const Location &loc) : AmfImporter(loc), archive(nullptr), zipfile(nullptr)
{
}

AmfImporterZIP::~AmfImporterZIP()
{
}

int AmfImporterZIP::read_callback(void *context, char *buffer, int len)
{
	AmfImporterZIP *importer = (AmfImporterZIP *)context;
	return zip_fread(importer->zipfile, buffer, len);
}

int AmfImporterZIP::close_callback(void *context)
{
	AmfImporterZIP *importer = (AmfImporterZIP *)context;
	return zip_fclose(importer->zipfile);
}

xmlTextReaderPtr AmfImporterZIP::createXmlReader(const char *filepath)
{
	archive = zip_open(filepath, 0, nullptr);
	if (archive) {
		// Separate the filename without using filesystem::path because that gives wide result on Windows TM
		const char *last_slash = strrchr(filepath,'/');
		const char *last_bslash = strrchr(filepath,'\\');
		if(last_bslash > last_slash)
			last_slash = last_bslash;
		const char *filename = last_slash ? last_slash + 1 : filepath;
		zipfile = zip_fopen(archive, filename, ZIP_FL_NODIR);
		if (zipfile == nullptr) {
			LOG(message_group::Warning,Location::NONE,"","Can't read file '%1$s' from zipped AMF '%2$s', import() at line %3$d",filename,filepath,this->loc.firstLine());
		}
		if ((zipfile == nullptr) && (zip_get_num_files(archive) == 1)) {
			LOG(message_group::Warning,Location::NONE,"","Trying to read single entry '%1$s'",zip_get_name(archive, 0, 0));
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

PolySet *import_amf(const std::string filename, const Location &loc) {
	AmfImporterZIP importer(loc);
	return importer.read(filename);
}

#else

PolySet *import_amf(const std::string filename, const Location &loc) {
	AmfImporter importer(loc);
	return importer.read(filename);
}

#endif
