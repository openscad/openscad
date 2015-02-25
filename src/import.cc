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

#include "module.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "evalcontext.h"
#include "builtin.h"
#include "dxfdata.h"
#include "printutils.h"
#include "fileutils.h"
#include "handle_dep.h" // handle_dep()

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

#include <sys/types.h>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope
#include "boosty.h"

#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

class ImportModule : public AbstractModule
{
public:
	import_type_e type;
	ImportModule(import_type_e type = TYPE_UNKNOWN) : type(type) { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

AbstractNode *ImportModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	AssignmentList args;
	args += Assignment("file"), Assignment("layer"), Assignment("convexity"), Assignment("origin"), Assignment("scale");
	args += Assignment("filename"), Assignment("layername");

  // FIXME: This is broken. Tag as deprecated and fix
	// Map old argnames to new argnames for compatibility
	// To fix: 
  // o after c.setVariables()
	//   - if "filename" in evalctx: deprecated-warning && v.set_variable("file", value);
	//   - if "layername" in evalctx: deprecated-warning && v.set_variable("layer", value);
#if 0
	std::vector<std::string> inst_argnames = inst->argnames;
	for (size_t i=0; i<inst_argnames.size(); i++) {
		if (inst_argnames[i] == "filename") inst_argnames[i] = "file";
		if (inst_argnames[i] == "layername") inst_argnames[i] = "layer";
	}
#endif

	Context c(ctx);
	c.setDocumentPath(evalctx->documentPath());
	c.setVariables(args, evalctx);
#if 0 && DEBUG
	c.dump(this, inst);
#endif

	ValuePtr v = c.lookup_variable("file");
	if (v->isUndefined()) {
		v = c.lookup_variable("filename");
		if (!v->isUndefined()) {
			printDeprecation("filename= is deprecated. Please use file=");
		}
	}
	std::string filename = lookup_file(v->isUndefined() ? "" : v->toString(), inst->path(), ctx->documentPath());
	import_type_e actualtype = this->type;
	if (actualtype == TYPE_UNKNOWN) {
		std::string extraw = boosty::extension_str(fs::path(filename));
		std::string ext = boost::algorithm::to_lower_copy(extraw);
		if (ext == ".stl") actualtype = TYPE_STL;
		else if (ext == ".off") actualtype = TYPE_OFF;
		else if (ext == ".dxf") actualtype = TYPE_DXF;
	}

	ImportNode *node = new ImportNode(inst, actualtype);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	node->filename = filename;
	Value layerval = *c.lookup_variable("layer", true);
	if (layerval.isUndefined()) {
		layerval = *c.lookup_variable("layername");
		if (!layerval.isUndefined()) {
			printDeprecation("layername= is deprecated. Please use layer=");
		}
	}
	node->layername = layerval.isUndefined() ? ""  : layerval.toString();
	node->convexity = c.lookup_variable("convexity", true)->toDouble();

	if (node->convexity <= 0) node->convexity = 1;

	ValuePtr origin = c.lookup_variable("origin", true);
	node->origin_x = node->origin_y = 0;
	origin->getVec2(node->origin_x, node->origin_y);

	node->scale = c.lookup_variable("scale", true)->toDouble();

	if (node->scale <= 0) node->scale = 1;

	return node;
}

#define STL_FACET_NUMBYTES 4*3*4+2
// as there is no 'float32_t' standard, we assume the systems 'float'
// is a 'binary32' aka 'single' standard IEEE 32-bit floating point type
union stl_facet {
	uint8_t data8[ STL_FACET_NUMBYTES ];
	uint32_t data32[4*3];
	struct facet_data {
	  float i, j, k;
	  float x1, y1, z1;
	  float x2, y2, z2;
	  float x3, y3, z3;
	  uint16_t attribute_byte_count;
	} data;
};

void uint32_byte_swap( uint32_t &x )
{
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
	x = __builtin_bswap32( x );
#elif defined(__clang__)
	x = __builtin_bswap32( x );
#elif defined(_MSC_VER)
	x = _byteswap_ulong( x );
#else
	uint32_t b1 = ( 0x000000FF & x ) << 24;
	uint32_t b2 = ( 0x0000FF00 & x ) << 8;
	uint32_t b3 = ( 0x00FF0000 & x ) >> 8;
	uint32_t b4 = ( 0xFF000000 & x ) >> 24;
	x = b1 | b2 | b3 | b4;
#endif
}

void read_stl_facet( std::ifstream &f, stl_facet &facet )
{
	f.read( (char*)facet.data8, STL_FACET_NUMBYTES );
#ifdef BOOST_BIG_ENDIAN
	for ( int i = 0; i < 12; i++ ) {
		uint32_byte_swap( facet.data32[ i ] );
	}
	// we ignore attribute byte count
#endif
}

/*!
	Will return an empty geometry if the import failed, but not NULL
*/
Geometry *ImportNode::createGeometry() const
{
	Geometry *g = NULL;

	switch (this->type) {
	case TYPE_STL: {
		PolySet *p = new PolySet(3);
		g = p;

		handle_dep((std::string)this->filename);
		// Open file and position at the end
		std::ifstream f(this->filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
		if (!f.good()) {
			PRINTB("WARNING: Can't open import file '%s'.", this->filename);
			return g;
		}

		boost::regex ex_sfe("solid|facet|endloop");
		boost::regex ex_outer("outer loop");
		boost::regex ex_vertex("vertex");
		boost::regex ex_vertices("\\s*vertex\\s+([^\\s]+)\\s+([^\\s]+)\\s+([^\\s]+)");

		bool binary = false;
		std::streampos file_size = f.tellg();
		f.seekg(80);
		if (f.good() && !f.eof()) {
			uint32_t facenum = 0;
			f.read((char *)&facenum, sizeof(uint32_t));
#ifdef BOOST_BIG_ENDIAN
			uint32_byte_swap( facenum );
#endif
			if (file_size ==  static_cast<std::streamoff>(80 + 4 + 50*facenum)) {
				binary = true;
			}
		}
		f.seekg(0);

		char data[5];
		f.read(data, 5);
		if (!binary && !f.eof() && f.good() && !memcmp(data, "solid", 5)) {
			int i = 0;
			double vdata[3][3];
			std::string line;
			std::getline(f, line);
			while (!f.eof()) {
				std::getline(f, line);
				boost::trim(line);
				if (boost::regex_search(line, ex_sfe)) {
					continue;
				}
				if (boost::regex_search(line, ex_outer)) {
					i = 0;
					continue;
				}
				boost::smatch results;
				if (boost::regex_search(line, results, ex_vertices)) {
					try {
						for (int v=0;v<3;v++) {
							vdata[i][v] = boost::lexical_cast<double>(results[v+1]);
						}
					}
					catch (const boost::bad_lexical_cast &blc) {
						PRINTB("WARNING: Can't parse vertex line '%s'.", line);
						i = 10;
						continue;
					}
					if (++i == 3) {
						p->append_poly();
						p->append_vertex(vdata[0][0], vdata[0][1], vdata[0][2]);
						p->append_vertex(vdata[1][0], vdata[1][1], vdata[1][2]);
						p->append_vertex(vdata[2][0], vdata[2][1], vdata[2][2]);
					}
				}
			}
		}
		else if (binary && !f.eof() && f.good())
		{
			f.ignore(80-5+4);
			while (1) {
				stl_facet facet;
				read_stl_facet( f, facet );
				if (f.eof()) break;
				p->append_poly();
				p->append_vertex(facet.data.x1, facet.data.y1, facet.data.z1);
				p->append_vertex(facet.data.x2, facet.data.y2, facet.data.z2);
				p->append_vertex(facet.data.x3, facet.data.y3, facet.data.z3);
			}
		}
	}
		break;
	case TYPE_OFF: {
		PolySet *p = new PolySet(3);
		g = p;
#ifdef ENABLE_CGAL
		CGAL_Polyhedron poly;
		std::ifstream file(this->filename.c_str(), std::ios::in | std::ios::binary);
		if (!file.good()) {
			PRINTB("WARNING: Can't open import file '%s'.", this->filename);
		}
		else {
			file >> poly;
			file.close();
			bool err = CGALUtils::createPolySetFromPolyhedron(poly, *p);
		}
#else
  PRINT("WARNING: OFF import requires CGAL.");
#endif
	}
		break;
	case TYPE_DXF: {
		DxfData dd(this->fn, this->fs, this->fa, this->filename, this->layername, this->origin_x, this->origin_y, this->scale);
		g = dd.toPolygon2d();
	}
		break;
	default:
		PRINTB("ERROR: Unsupported file format while trying to import file '%s'", this->filename);
		g = new PolySet(0);
	}

	if (g) g->setConvexity(this->convexity);
	return g;
}

std::string ImportNode::toString() const
{
	std::stringstream stream;
	fs::path path((std::string)this->filename);

	stream << this->name();
	stream << "(file = " << this->filename << ", "
		"layer = " << QuotedString(this->layername) << ", "
		"origin = [" << std::dec << this->origin_x << ", " << this->origin_y << "], "
		"scale = " << this->scale << ", "
		"convexity = " << this->convexity << ", "
		"$fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs
#ifndef OPENSCAD_TESTING
  // timestamp is needed for caching, but disturbs the test framework
				 << ", " "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0)
#endif
				 << ")";


	return stream.str();
}

std::string ImportNode::name() const
{
	return "import";
}

void register_builtin_import()
{
	Builtins::init("import_stl", new ImportModule(TYPE_STL));
	Builtins::init("import_off", new ImportModule(TYPE_OFF));
	Builtins::init("import_dxf", new ImportModule(TYPE_DXF));
	Builtins::init("import", new ImportModule());
}
