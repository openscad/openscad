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
#include "context.h"
#include "builtin.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "printutils.h"
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
using namespace boost::filesystem;
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope
#include "boosty.h"

class ImportModule : public AbstractModule
{
public:
	import_type_e type;
	ImportModule(import_type_e type = TYPE_UNKNOWN) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *ImportModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	std::vector<std::string> argnames;
	argnames += "file", "layer", "convexity", "origin", "scale";
	std::vector<Expression*> argexpr;

	// Map old argnames to new argnames for compatibility
	std::vector<std::string> inst_argnames = inst->argnames;
	for (size_t i=0; i<inst_argnames.size(); i++) {
		if (inst_argnames[i] == "filename") inst_argnames[i] = "file";
		if (inst_argnames[i] == "layername") inst_argnames[i] = "layer";
	}

	Context c(ctx);
	c.args(argnames, argexpr, inst_argnames, inst->argvalues);

	Value v = c.lookup_variable("file");
	std::string filename = c.getAbsolutePath(v.isUndefined() ? "" : v.toString());
	import_type_e actualtype = this->type;
	if (actualtype == TYPE_UNKNOWN) {
		std::string extraw = boosty::extension_str( path(filename) );
		std::string ext = boost::algorithm::to_lower_copy( extraw );
		if (ext == ".stl") actualtype = TYPE_STL;
		else if (ext == ".off") actualtype = TYPE_OFF;
		else if (ext == ".dxf") actualtype = TYPE_DXF;
	}

	ImportNode *node = new ImportNode(inst, actualtype);

	node->fn = c.lookup_variable("$fn").toDouble();
	node->fs = c.lookup_variable("$fs").toDouble();
	node->fa = c.lookup_variable("$fa").toDouble();

	node->filename = filename;
	Value layerval = c.lookup_variable("layer", true);
	node->layername = layerval.isUndefined() ? ""  : layerval.toString();
	node->convexity = c.lookup_variable("convexity", true).toDouble();

	if (node->convexity <= 0) node->convexity = 1;

	Value origin = c.lookup_variable("origin", true);
	node->origin_x = node->origin_y = 0;
	origin.getVec2(node->origin_x, node->origin_y);

	node->scale = c.lookup_variable("scale", true).toDouble();

	if (node->scale <= 0)
		node->scale = 1;

	return node;
}

PolySet *ImportNode::evaluate_polyset(class PolySetEvaluator *) const
{
	PolySet *p = NULL;

	if (this->type == TYPE_STL)
	{
		handle_dep((std::string)this->filename);
		std::ifstream f(this->filename.c_str(), std::ios::in | std::ios::binary);
		if (!f.good()) {
			PRINTB("WARNING: Can't open import file '%s'.", this->filename);
			return p;
		}

		p = new PolySet();

		boost::regex ex_sfe("solid|facet|endloop");
		boost::regex ex_outer("outer loop");
		boost::regex ex_vertex("vertex");
		boost::regex ex_vertices("\\s*vertex\\s+([^\\s]+)\\s+([^\\s]+)\\s+([^\\s]+)");

		char data[5];
		f.read(data, 5);
		if (!f.eof() && !memcmp(data, "solid", 5)) {
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
					catch (boost::bad_lexical_cast &blc) {
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
		else
		{
			f.ignore(80-5+4);
			while (1) {
#ifdef _MSC_VER
#pragma pack(push,1)
#endif
				struct {
					float i, j, k;
					float x1, y1, z1;
					float x2, y2, z2;
					float x3, y3, z3;
					unsigned short acount;
				}
#ifdef __GNUC__
				__attribute__ ((packed))
#endif
				stldata;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

				f.read((char*)&stldata, sizeof(stldata));
				if (f.eof()) break;
				p->append_poly();
				p->append_vertex(stldata.x1, stldata.y1, stldata.z1);
				p->append_vertex(stldata.x2, stldata.y2, stldata.z2);
				p->append_vertex(stldata.x3, stldata.y3, stldata.z3);
			}
		}
	}

	else if (this->type == TYPE_OFF)
	{
#ifdef ENABLE_CGAL
		CGAL_Polyhedron poly;
		std::ifstream file(this->filename.c_str(), std::ios::in | std::ios::binary);
		file >> poly;
		file.close();
		
		p = createPolySetFromPolyhedron(poly);
#else
  PRINT("WARNING: OFF import requires CGAL.");
#endif
	}

	else if (this->type == TYPE_DXF)
	{
		p = new PolySet();
		DxfData dd(this->fn, this->fs, this->fa, this->filename, this->layername, this->origin_x, this->origin_y, this->scale);
		p->is2d = true;
		dxf_tesselate(p, dd, 0, true, false, 0);
		dxf_border_to_ps(p, dd);
	}
	else 
	{
		PRINTB("ERROR: Unsupported file format while trying to import file '%s'", this->filename);
	}

	if (p) p->convexity = this->convexity;
	return p;
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
