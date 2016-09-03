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

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

#include <sys/types.h>
#include <map>
#include <fstream>
#include <assert.h>
#include <libxml/xmlreader.h>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

fs::path path("/");
const std::string text_node("#text");
const std::string object("/amf/object");
const std::string coordinates("/amf/object/mesh/vertices/vertex/coordinates");
const std::string coordinates_x = coordinates + "/x";
const std::string coordinates_y = coordinates + "/y";
const std::string coordinates_z = coordinates + "/z";
const std::string triangle("/amf/object/mesh/volume/triangle");
const std::string triangle_v1 = triangle + "/v1";
const std::string triangle_v2 = triangle + "/v2";
const std::string triangle_v3 = triangle + "/v3";

typedef void (*cb_func)(const xmlChar *);

static PolySet *polySet;
static Eigen::Vector3d vertex;
static int idx_v1, idx_v2, idx_v3;
static std::vector<Eigen::Vector3d> vertex_list;
static std::vector<PolySet *> polySets;

static std::map<const std::string, cb_func> funcs;
static std::map<const std::string, cb_func> start_funcs;
static std::map<const std::string, cb_func> end_funcs;

static void set_x(const xmlChar *value)
{
	double x = atof((const char *) value);
	vertex = Eigen::Vector3d(x, vertex.y(), vertex.z());
}

static void set_y(const xmlChar *value)
{
	double y = atof((const char *) value);
	vertex = Eigen::Vector3d(vertex.x(), y, vertex.z());
}

static void set_z(const xmlChar *value)
{
	double z = atof((const char *) value);
	vertex = Eigen::Vector3d(vertex.x(), vertex.y(), z);
}

static void set_v1(const xmlChar *value)
{
	idx_v1 = atof((const char *)value);
}

static void set_v2(const xmlChar *value)
{
	idx_v2 = atof((const char *)value);
}

static void set_v3(const xmlChar *value)
{
	idx_v3 = atof((const char *)value);
}

static void start_object(const xmlChar *) {
	polySet = new PolySet(3);
}

static void end_vertex(const xmlChar *)
{
	vertex_list.push_back(vertex);
}

static void end_triangle(const xmlChar *)
{
	polySet->append_poly();
	polySet->append_vertex(vertex_list[idx_v1].x(), vertex_list[idx_v1].y(), vertex_list[idx_v1].z());
	polySet->append_vertex(vertex_list[idx_v2].x(), vertex_list[idx_v2].y(), vertex_list[idx_v2].z());
	polySet->append_vertex(vertex_list[idx_v3].x(), vertex_list[idx_v3].y(), vertex_list[idx_v3].z());
}

static void end_object(const xmlChar *)
{
	polySets.push_back(polySet);
	vertex_list.clear();
	polySet = NULL;
}

static void processNode(xmlTextReaderPtr reader)
{
	const char *name = reinterpret_cast<const char *> (xmlTextReaderName(reader));
	if (name == NULL)
		name = reinterpret_cast<const char *> (xmlStrdup(BAD_CAST "--"));

	xmlChar *value = xmlTextReaderValue(reader);
	int node_type = xmlTextReaderNodeType(reader);
	switch (node_type) {
	case XML_READER_TYPE_ELEMENT:
	{
		path /= name;
		cb_func startFunc = start_funcs[path.string()];
		if (startFunc) {
			startFunc(NULL);
		}
	}
		break;
	case XML_READER_TYPE_END_ELEMENT:
	{
		cb_func f1 = end_funcs[path.string()];
		if (f1) {
			f1(value);
		}
		path = path.parent_path();
	}
		break;
	case XML_READER_TYPE_TEXT:
	{
		cb_func f = funcs[path.string()];
		if (f) {
			f(value);
		}
	}
		break;
	}

	xmlFree(value);
	xmlFree((void *) (name));
}

static int streamFile(const char *filename)
{
	xmlTextReaderPtr reader;
	int ret;

	reader = xmlNewTextReaderFilename(filename);
	xmlTextReaderSetParserProp(reader, XML_PARSER_SUBST_ENTITIES, 1);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			processNode(reader);
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			printf("%s : failed to parse\n", filename);
		}
	} else {
		printf("Unable to open %s\n", filename);
	}
	return 0;
}

PolySet *import_amf(const std::string filename) {
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

	PolySet *p;
#ifdef ENABLE_CGAL
	Geometry::Geometries children;
	for (std::vector<PolySet *>::iterator it = polySets.begin();it != polySets.end();it++) {
		children.push_back(std::make_pair((const AbstractNode*)NULL,  shared_ptr<const Geometry>(*it)));
	}
	CGAL_Nef_polyhedron *N = CGALUtils::applyOperator(children, OPENSCAD_UNION);
	PolySet *result = new PolySet(3);
	if (CGALUtils::createPolySetFromNefPolyhedron3(*N->p3, *result)) {
		delete result;
	} else {
		p = result;
	}
#else
	p = new PolySet(3);
#endif
	polySets.clear();
	return p;
}
