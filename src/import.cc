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
#include "builtin.h"
#include "children.h"
#include "dxfdata.h"
#include "parameters.h"
#include "printutils.h"
#include "fileutils.h"
#include "feature.h"
#include "handle_dep.h"
#include "boost-utils.h"
#include <sys/types.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <cstdint>

extern PolySet * import_amf(std::string, const Location &loc);
extern Geometry * import_3mf(const std::string &, const Location &loc);

static AbstractNode* do_import(const ModuleInstantiation *inst, Arguments arguments, Children children, ImportType type)
{
	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",inst->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
		{"file", "layer", "convexity", "origin", "scale"},
		{"width", "height", "filename", "layername", "center", "dpi"}
	);

	const auto &v = parameters["file"];
	std::string filename;
	if (v.isDefined()) {
		filename = lookup_file(v.isUndefined() ? "" : v.toString(), inst->location().filePath().parent_path().string(), parameters.documentRoot());
	} else {
		const auto &filename_val = parameters["filename"];
		if (!filename_val.isUndefined()) {
			LOG(message_group::Deprecated,Location::NONE,"","filename= is deprecated. Please use file=");
		}
		filename = lookup_file(filename_val.isUndefined() ? "" : filename_val.toString(), inst->location().filePath().parent_path().string(), parameters.documentRoot());
	}
	if (!filename.empty()) handle_dep(filename);
	ImportType actualtype = type;
	if (actualtype == ImportType::UNKNOWN) {
		std::string extraw = fs::path(filename).extension().generic_string();
		std::string ext = boost::algorithm::to_lower_copy(extraw);
		if (ext == ".stl") actualtype = ImportType::STL;
		else if (ext == ".off") actualtype = ImportType::OFF;
		else if (ext == ".dxf") actualtype = ImportType::DXF;
		else if (ext == ".nef3") actualtype = ImportType::NEF3;
		else if (ext == ".3mf") actualtype = ImportType::_3MF;
		else if (ext == ".amf") actualtype = ImportType::AMF;
		else if (ext == ".svg") actualtype = ImportType::SVG;
	}

	auto node = new ImportNode(inst, actualtype);

	node->fn = parameters["$fn"].toDouble();
	node->fs = parameters["$fs"].toDouble();
	node->fa = parameters["$fa"].toDouble();

	node->filename = filename;
	const auto &layerval = parameters["layer"];
	if (layerval.isDefined()) {
		node->layername = layerval.toString();
	} else {
		const auto &layername = parameters["layername"];
		if (layername.isDefined()) {
			LOG(message_group::Deprecated,Location::NONE,"","layername= is deprecated. Please use layer=");
			node->layername = layername.toString();
		} else {
			node->layername = "";
		}
	}
	node->convexity = (int)parameters["convexity"].toDouble();

	if (node->convexity <= 0) node->convexity = 1;

	const auto &origin = parameters["origin"];
	node->origin_x = node->origin_y = 0;
	bool originOk = origin.getVec2(node->origin_x, node->origin_y);
	originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
	if(origin.isDefined() && !originOk){
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"linear_extrude(..., origin=%1$s) could not be converted",origin.toEchoString());
	}

	const auto &center = parameters["center"];
	node->center = center.type() == Value::Type::BOOL ? center.toBool() : false;

	node->scale = parameters["scale"].toDouble();
	if (node->scale <= 0) node->scale = 1;

	node->dpi = ImportNode::SVG_DEFAULT_DPI;
	const auto &dpi = parameters["dpi"];
	if (dpi.type() == Value::Type::NUMBER) {
		double val = dpi.toDouble();
		if (val < 0.001) {
			std::string filePath = boostfs_uncomplete(inst->location().filePath(),parameters.documentRoot()).generic_string();
			LOG(message_group::Warning,Location::NONE,"",
				"Invalid dpi value giving, using default of %1$f dpi. Value must be positive and >= 0.001, file %2$s, import() at line %3$d",
				origin.toEchoString(),filePath,filePath,inst->location().firstLine()
			);
		} else {
			node->dpi = val;
		}
	}

	node->width = parameters.get("width", -1);
	node->height = parameters.get("height", -1);

	return node;
}

static AbstractNode* builtin_import(const ModuleInstantiation *inst, Arguments arguments, Children children)
	{ return do_import(inst, std::move(arguments), std::move(children), ImportType::UNKNOWN); }

static AbstractNode* builtin_import_stl(const ModuleInstantiation *inst, Arguments arguments, Children children)
	{ return do_import(inst, std::move(arguments), std::move(children), ImportType::STL); }

static AbstractNode* builtin_import_off(const ModuleInstantiation *inst, Arguments arguments, Children children)
	{ return do_import(inst, std::move(arguments), std::move(children), ImportType::OFF); }

static AbstractNode* builtin_import_dxf(const ModuleInstantiation *inst, Arguments arguments, Children children)
	{ return do_import(inst, std::move(arguments), std::move(children), ImportType::DXF); }



/*!
	Will return an empty geometry if the import failed, but not nullptr
*/
const Geometry *ImportNode::createGeometry() const
{
	Geometry *g = nullptr;
	auto loc = this->modinst->location();

	switch (this->type) {
	case ImportType::STL: {
		g = import_stl(this->filename, loc);
		break;
	}
	case ImportType::AMF: {
		g = import_amf(this->filename, loc);
		break;
	}
	case ImportType::_3MF: {
		g = import_3mf(this->filename, loc);
		break;
	}
	case ImportType::OFF: {
		g = import_off(this->filename, loc);
		break;
	}
	case ImportType::SVG: {
		g = import_svg(this->filename, this->dpi, this->center, loc);
 		break;
	}
	case ImportType::DXF: {
		DxfData dd(this->fn, this->fs, this->fa, this->filename, this->layername, this->origin_x, this->origin_y, this->scale);
		g = dd.toPolygon2d();
		break;
	}
#ifdef ENABLE_CGAL
	case ImportType::NEF3: {
		g = import_nef3(this->filename, loc);
		break;
	}
#endif
	default:
		LOG(message_group::Error,Location::NONE,"","Unsupported file format while trying to import file '%1$s', import() at line %2$d",this->filename,loc.firstLine());
		g = new PolySet(3);
	}

	if (g) g->setConvexity(this->convexity);
	return g;
}

std::string ImportNode::toString() const
{
	std::ostringstream stream;
	fs::path path((std::string)this->filename);

	stream << this->name();
	stream << "(file = " << this->filename
		<< ", layer = " << QuotedString(this->layername)
		<< ", origin = [" << std::dec << this->origin_x << ", " << this->origin_y << "]";
	if (this->type == ImportType::SVG) {
		stream << ", center = " << (this->center ? "true" : "false")
			   << ", dpi = " << this->dpi;
	}
	stream << ", scale = " << this->scale
		<< ", convexity = " << this->convexity
		<< ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs
		<< ", timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0)
		<< ")";

	return stream.str();
}

std::string ImportNode::name() const
{
	return "import";
}

void register_builtin_import()
{
	Builtins::init("import_stl", new BuiltinModule(builtin_import_stl));
	Builtins::init("import_off", new BuiltinModule(builtin_import_off));
	Builtins::init("import_dxf", new BuiltinModule(builtin_import_dxf));

	Builtins::init("import", new BuiltinModule(builtin_import),
				{
					"import(string, [number, [number]])",
				});
}
