/*
 * The MIT License
 *
 * Copyright (c) 2016-2018, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <map>
#include <stack>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
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

#define SVG_DEBUG 0

static bool in_defs = false;
static shapes_list_t stack;
static shapes_list_t *shape_list;

#if SVG_DEBUG
static std::string dump_stack() {
	bool first = true;
	std::stringstream s;
	s << "[";
	for (const auto &shape : stack) {
		s << (first ? "" : "|") << shape->get_name();
		first = false;
	}
	return s.str() + "]";
}
#endif

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
#if SVG_DEBUG
		printf("XML_READER_TYPE_ELEMENT (%s %s): %d %d %s\n",
			dump_stack().c_str(), name,
			xmlTextReaderDepth(reader),
			xmlTextReaderNodeType(reader),
			value);
#endif

		if (std::string("defs") == name) {
			in_defs = true;
		}
		
		auto s = shared_ptr<shape>(shape::create_from_name(name));
		if (!in_defs && s) {
			attr_map_t attrs = read_attributes(reader);
			s->set_attrs(attrs);
			shape_list->push_back(s);
			if (!stack.empty()) {
				stack.back()->add_child(s.get());
			}
			if (s->is_container()) {
				stack.push_back(s);
			}
			s->apply_transform();
		}
	}	
	if (!isEmpty) {
		break;
	}
	/* fall through */
	case XML_READER_TYPE_END_ELEMENT:
	{
		if (std::string("defs") == name) {
			in_defs = false;
		}
		if (in_defs) {
			return;
		}

		if (std::string("g") == name) {
			stack.pop_back();
		} else if (std::string("tspan") == name) {
			stack.pop_back();
		} else if (std::string("text") == name) {
			stack.pop_back();
		}
#if SVG_DEBUG
		printf("XML_READER_TYPE_END_ELEMENT (%s %s): %d %d %s\n",
			dump_stack().c_str(), name,
			xmlTextReaderDepth(reader),
			xmlTextReaderNodeType(reader),
			value);
#endif
	}
		break;
	case XML_READER_TYPE_TEXT:
	{
		attr_map_t attrs;
		attrs["text"] = reinterpret_cast<const char *>(value);
		auto s = shared_ptr<shape>(shape::create_from_name("data"));
		s->set_attrs(attrs);
		shape_list->push_back(s);
		if (!stack.empty()) {
			stack.back()->add_child(s.get());
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
			throw SvgException((boost::format("Error parsing file '%1%'") % filename).str());
		}
	} else {
		throw SvgException((boost::format("Can't open file '%1%'") % filename).str());
	}
	return 0;
}

void dump(int idx, shape *s) {
	for (int a = 0;a < idx;a++) {
		std::cout << "  ";
	}
	std::cout << "=> " << s->dump() << std::endl;
	for (const auto& c : s->get_children()) {
		dump(idx + 1, c);
	} 
}

shapes_list_t *
libsvg_read_file(const char *filename)
{
	shape_list = new shapes_list_t();
	streamFile(filename);

//#ifdef DEBUG
//	if (!shape_list->empty()) {
//		dump(0, shape_list->front().get());
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
