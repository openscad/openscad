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

class ImportModule : public AbstractModule
{
public:
	ImportType type;
	ImportModule(ImportType type = ImportType::UNKNOWN) : type(type) { }
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *ImportModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
  AssignmentList args{
    assignment("file"), assignment("layer"), assignment("convexity"),
		assignment("origin"), assignment("scale")
	};

	AssignmentList optargs{
		assignment("width"), assignment("height"),
		assignment("filename"), assignment("layername"), assignment("center"), assignment("dpi")
	};

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setDocumentPath(evalctx->documentPath());
	c->setVariables(evalctx, args, optargs);
#if 0 && DEBUG
	c.dump(this, inst);
#endif

	const auto &v = c->lookup_variable("file", true);
	std::string filename;
	if (v.isDefined()) {
		filename = lookup_file(v.isUndefined() ? "" : v.toString(), inst->path(), ctx->documentPath());
	} else {
		const auto &filename_val = c->lookup_variable("filename", true);
		if (!filename_val.isUndefined()) {
			LOG(message_group::Deprecated,Location::NONE,"","filename= is deprecated. Please use file=");
		}
		filename = lookup_file(filename_val.isUndefined() ? "" : filename_val.toString(), inst->path(), ctx->documentPath());
	}
	if (!filename.empty()) handle_dep(filename);
	ImportType actualtype = this->type;
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

	auto node = new ImportNode(inst, evalctx, actualtype);

	node->fn = c->lookup_variable("$fn").toDouble();
	node->fs = c->lookup_variable("$fs").toDouble();
	node->fa = c->lookup_variable("$fa").toDouble();

	node->filename = filename;
	const auto &layerval = c->lookup_variable("layer", true);
	if (layerval.isDefined()) {
		node->layername = layerval.toString();
	} else {
		const auto &layername = c->lookup_variable("layername", true);
		if (layername.isDefined()) {
			LOG(message_group::Deprecated,Location::NONE,"","layername= is deprecated. Please use layer=");
			node->layername = layername.toString();
		} else {
			node->layername = "";
		}
	}
	node->convexity = (int)c->lookup_variable("convexity", true).toDouble();

	if (node->convexity <= 0) node->convexity = 1;

	const auto &origin = c->lookup_variable("origin", true);
	node->origin_x = node->origin_y = 0;
	bool originOk = origin.getVec2(node->origin_x, node->origin_y);
	originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
	if(origin.isDefined() && !originOk){
		LOG(message_group::Warning,evalctx->loc,ctx->documentPath(),"linear_extrude(..., origin=%1$s) could not be converted",origin.toEchoString());
	}

	const auto &center = c->lookup_variable("center", true);
	node->center = center.type() == Value::Type::BOOL ? center.toBool() : false;

	node->scale = c->lookup_variable("scale", true).toDouble();
	if (node->scale <= 0) node->scale = 1;

	node->dpi = ImportNode::SVG_DEFAULT_DPI;
	const auto &dpi = c->lookup_variable("dpi", true);
	if (dpi.type() == Value::Type::NUMBER) {
		double val = dpi.toDouble();
		if (val < 0.001) {
		std::string filePath = boostfs_uncomplete(inst->location().filePath(),ctx->documentPath()).generic_string();
		LOG(message_group::Warning,Location::NONE,"",
			"Invalid dpi value giving, using default of %1$f dpi. Value must be positive and >= 0.001, file %2$s, import() at line %3$d",
			origin.toEchoString(),filePath,filePath,inst->location().firstLine());
		} else {
			node->dpi = val;
		}
	}

	const auto &width = c->lookup_variable("width", true);
	const auto &height = c->lookup_variable("height", true);
	node->width = (width.type() == Value::Type::NUMBER) ? width.toDouble() : -1;
	node->height = (height.type() == Value::Type::NUMBER) ? height.toDouble() : -1;

	return node;
}

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
	Builtins::init("import_stl", new ImportModule(ImportType::STL));
	Builtins::init("import_off", new ImportModule(ImportType::OFF));
	Builtins::init("import_dxf", new ImportModule(ImportType::DXF));

	Builtins::init("import", new ImportModule(),
				{
					"import(string, [number, [number]])",
				});
}
