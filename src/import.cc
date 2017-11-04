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

#include "import.h"
#include "importnode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "polyset.h"
#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#endif
#include "Polygon2d.h"
#include "evalcontext.h"
#include "builtin.h"
#include "dxfdata.h"
#include "printutils.h"
#include "fileutils.h"
#include "feature.h"
#include "handle_dep.h"

#include <sys/types.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/detail/endian.hpp>
#include <cstdint>

extern PolySet *import_amf(std::string);

class ImportModule : public AbstractModule
{
public:
	ImportType type;
	ImportModule(ImportType type = ImportType::UNKNOWN) : type(type) { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

AbstractNode *ImportModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	AssignmentList args{
		Assignment("file"), Assignment("layer"), Assignment("convexity"),
		Assignment("origin"), Assignment("scale"), Assignment("filename"),
		Assignment("layername")
	};

	// FIXME: This is broken. Tag as deprecated and fix
	// Map old argnames to new argnames for compatibility
	// To fix:
	// o after c.setVariables()
	//   - if "filename" in evalctx: deprecated-warning && v.set_variable("file", value);
	//   - if "layername" in evalctx: deprecated-warning && v.set_variable("layer", value);
#if 0
	std::vector<std::string> inst_argnames = inst->argnames;
	for (size_t i = 0; i < inst_argnames.size(); i++) {
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

	auto v = c.lookup_variable("file");
	if (v->isUndefined()) {
		v = c.lookup_variable("filename");
		if (!v->isUndefined()) {
			printDeprecation("filename= is deprecated. Please use file=");
		}
	}
	std::string filename = lookup_file(v->isUndefined() ? "" : v->toString(), inst->path(), ctx->documentPath());
	if (!filename.empty()) handle_dep(filename);
	ImportType actualtype = this->type;
	if (actualtype == ImportType::UNKNOWN) {
		std::string extraw = fs::path(filename).extension().generic_string();
		std::string ext = boost::algorithm::to_lower_copy(extraw);
		if (ext == ".stl") actualtype = ImportType::STL;
		else if (ext == ".off") actualtype = ImportType::OFF;
		else if (ext == ".dxf") actualtype = ImportType::DXF;
		else if (ext == ".nef3") actualtype = ImportType::NEF3;
		else if (Feature::ExperimentalAmfImport.is_enabled() && ext == ".amf") actualtype = ImportType::AMF;
		else if (Feature::ExperimentalSvgImport.is_enabled() && ext == ".svg") actualtype = ImportType::SVG;
	}

	auto node = new ImportNode(inst, actualtype);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	node->filename = filename;
	auto layerval = *c.lookup_variable("layer", true);
	if (layerval.isUndefined()) {
		layerval = *c.lookup_variable("layername");
		if (!layerval.isUndefined()) {
			printDeprecation("layername= is deprecated. Please use layer=");
		}
	}
	node->layername = layerval.isUndefined() ? ""  : layerval.toString();
	node->convexity = c.lookup_variable("convexity", true)->toDouble();

	if (node->convexity <= 0) node->convexity = 1;

	auto origin = c.lookup_variable("origin", true);
	node->origin_x = node->origin_y = 0;
	origin->getVec2(node->origin_x, node->origin_y);

	node->scale = c.lookup_variable("scale", true)->toDouble();

	if (node->scale <= 0) node->scale = 1;

	auto width = c.lookup_variable("width", true);
	auto height = c.lookup_variable("height", true);
	node->width = (width->type() == Value::ValueType::NUMBER) ? width->toDouble() : -1;
	node->height = (height->type() == Value::ValueType::NUMBER) ? height->toDouble() : -1;

	return node;
}

/*!
   Will return an empty geometry if the import failed, but not nullptr
 */
const Geometry *ImportNode::createGeometry() const
{
	Geometry *g = nullptr;

	switch (this->type) {
	case ImportType::STL: {
		g = import_stl(this->filename);
		break;
	}
	case ImportType::AMF: {
		g = import_amf(this->filename);
		break;
	}
	case ImportType::OFF: {
		g = import_off(this->filename);
		break;
	}
	case ImportType::SVG: {
		g = import_svg(this->filename);
		break;
	}
	case ImportType::DXF: {
		DxfData dd(this->fn, this->fs, this->fa, this->filename, this->layername, this->origin_x, this->origin_y, this->scale);
		g = dd.toPolygon2d();
		break;
	}
#ifdef ENABLE_CGAL
	case ImportType::NEF3: {
		g = import_nef3(this->filename);
		break;
	}
#endif
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
				 << ", " "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0)
				 << ")";


	return stream.str();
}

std::string ImportNode::name() const
{
	return "import";
}

void register_builtin_import()
{
	Builtins::init("import_stl", new ImportModule(ImportType::STL));
	Builtins::init("import_off", new ImportModule(ImportType::OFF));
	Builtins::init("import_dxf", new ImportModule(ImportType::DXF));
	Builtins::init("import", new ImportModule());
}
