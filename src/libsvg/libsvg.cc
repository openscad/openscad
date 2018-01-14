#include <map>
#include <stack>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/filesystem.hpp>
#include <libxml/xmlreader.h>

#include "libsvg.h"

#include "shape.h"
#include "circle.h"
#include "ellipse.h"
#include "line.h"
#include "polygon.h"
#include "polyline.h"
#include "rect.h"

namespace fs = boost::filesystem;

namespace libsvg {

fs::path path("/");

typedef void (*cb_func)(const xmlChar *);

std::map<const std::string, cb_func> funcs;
std::map<const std::string, cb_func> end_funcs;

static bool in_defs = false;
static std::stack<shape *> shapes;
static shapes_list_t *shape_list;

attr_map_t read_attributes(xmlTextReaderPtr reader)
{
	attr_map_t attrs;
	int attr_count = xmlTextReaderAttributeCount(reader);
	for (int idx = 0;idx < attr_count;idx++) {
		xmlTextReaderMoveToAttributeNo(reader, idx);
		const char *name = reinterpret_cast<const char *> (xmlTextReaderName(reader));
		const char *value = reinterpret_cast<const char *> (xmlTextReaderValue(reader));
		attrs[name] = value;
	}
	return attrs;
}

void processNode(xmlTextReaderPtr reader)
{
	const char *name = reinterpret_cast<const char *> (xmlTextReaderName(reader));
	if (name == nullptr) name = reinterpret_cast<const char *> (xmlStrdup(BAD_CAST "--"));

	bool isEmpty;
	xmlChar *value = xmlTextReaderValue(reader);
	int node_type = xmlTextReaderNodeType(reader);
	switch (node_type) {
	case XML_READER_TYPE_ELEMENT:
		isEmpty = xmlTextReaderIsEmptyElement(reader);
	{
		path /= name;
		
		if (std::string("defs") == name) {
			in_defs = true;
		}
		
		shape *s = shape::create_from_name(name);
		if (!in_defs && s) {
			attr_map_t attrs = read_attributes(reader);
			s->set_attrs(attrs);
			shape_list->push_back(s);
			if (!shapes.empty()) {
				shapes.top()->add_child(s);
			}
			if (s->is_container()) {
				shapes.push(s);
			}
			s->apply_transform();
		}
	}	
	if (!isEmpty) {
		break;
	}
	case XML_READER_TYPE_END_ELEMENT:
	{
		if (std::string("defs") == name) {
			in_defs = false;
		} else if (std::string("g") == name) {
			shapes.pop();
		}
		cb_func f1 = end_funcs[path.string()];
		if (f1) {
			f1(value);
		}
		path = path.parent_path();
	}
		break;
	case XML_READER_TYPE_TEXT:
	{
//		printf("%d %d %s %s\n",
//			xmlTextReaderDepth(reader),
//			xmlTextReaderNodeType(reader),
//			path.c_str(),
//			value);
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

int streamFile(const char *filename)
{
	xmlTextReaderPtr reader;

	in_defs = false;
	reader = xmlNewTextReaderFilename(filename);
	xmlTextReaderSetParserProp(reader, XML_PARSER_SUBST_ENTITIES, 1);
	if (reader != nullptr) {
		int ret = xmlTextReaderRead(reader);
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

void dump(int idx, shape *s) {
	for (int a = 0;a < idx;a++) {
		std::cout << "  ";
	}
	std::cout << "=> " << *s << std::endl;
	std::vector<shape *> children = s->get_children();
	for (std::vector<shape *>::iterator it = children.begin();it != children.end();it++) {
		dump(idx + 1, *it);
	} 
}

shapes_list_t *
libsvg_read_file(const char *filename)
{
	shape_list = new shapes_list_t();
	streamFile(filename);

//#ifdef DEBUG
//	if (!shape_list->empty()) {
//		dump(0, (*shape_list)[0]);
//	}
//#endif

	return shape_list;
}

void
libsvg_free(shapes_list_t *shapes)
{
	delete shapes;
}

}
