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

#include "linearextrudenode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "children.h"
#include "parameters.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "calc.h"
#include "polyset.h"
#include "handle_dep.h"

#include <cmath>
#include <sstream>
#include "boost-utils.h"
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

/*
 * Historic linear_extrude argument parsing is quirky. To remain bug-compatible,
 * try two different parses depending on conditions.
 */
Parameters parse_parameters(Arguments arguments, const Location& location)
{
	{
		Parameters normal_parse = Parameters::parse(arguments.clone(), location,
			{"file", "layer", "height", "origin", "scale", "center", "twist", "slices", "segments"},
			{"convexity"}
		);
		if (normal_parse["height"].isDefined()) {
			return normal_parse;
		}
		if (!(arguments.size() > 0 && !arguments[0].name && arguments[0]->type() == Value::Type::NUMBER)) {
			return normal_parse;
		}
	}
	
	// if height not given, and first argument is a number,
	// then assume it should be the height.
	return Parameters::parse(std::move(arguments), location,
		{"height", "file", "layer", "origin", "scale", "center", "twist", "slices", "segments"},
		{"convexity"}
	);
}

static AbstractNode* builtin_linear_extrude(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new LinearExtrudeNode(inst);

	Parameters parameters = parse_parameters(std::move(arguments), inst->location());

	node->fn = parameters["$fn"].toDouble();
	node->fs = parameters["$fs"].toDouble();
	node->fa = parameters["$fa"].toDouble();

	if (!parameters["file"].isUndefined() && parameters["file"].type() == Value::Type::STRING) {
		LOG(message_group::Deprecated,Location::NONE,"","Support for reading files in linear_extrude will be removed in future releases. Use a child import() instead.");
		auto filename = lookup_file(parameters["file"].toString(), inst->location().filePath().parent_path().string(), parameters.documentRoot());
		node->filename = filename;
		handle_dep(filename);
	}

	if (parameters["height"].isDefined()) {
		parameters["height"].getFiniteDouble(node->height);
	}

	node->layername = parameters["layer"].isUndefined() ? "" : parameters["layer"].toString();

	double tmp_convexity = 0.0;
	parameters["convexity"].getFiniteDouble(tmp_convexity);
	node->convexity = static_cast<int>(tmp_convexity);

	bool originOk = parameters["origin"].getVec2(node->origin_x, node->origin_y);
	originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
	if(parameters["origin"].isDefined() && !originOk){
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"linear_extrude(..., origin=%1$s) could not be converted",parameters["origin"].toEchoString());
	}
	node->scale_x = node->scale_y = 1;
	bool scaleOK = parameters["scale"].getFiniteDouble(node->scale_x);
	scaleOK &= parameters["scale"].getFiniteDouble(node->scale_y);
	scaleOK |= parameters["scale"].getVec2(node->scale_x, node->scale_y, true);
	if((parameters["scale"].isDefined()) && (!scaleOK || !std::isfinite(node->scale_x) || !std::isfinite(node->scale_y))) {
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"linear_extrude(..., scale=%1$s) could not be converted",parameters["scale"].toEchoString());
	}

	if (parameters["center"].type() == Value::Type::BOOL)
		node->center = parameters["center"].toBool();

	if (node->height <= 0) node->height = 0;

	if (node->convexity <= 0)
		node->convexity = 1;

	if (node->scale_x < 0) node->scale_x = 0;
	if (node->scale_y < 0) node->scale_y = 0;

	double slicesVal = 0;
	parameters["slices"].getFiniteDouble(slicesVal);
	node->slices = static_cast<int>(slicesVal);
	if (node->slices > 0) {
		node->has_slices = true;
	} 

	double segmentsVal = 0;
	if (parameters["segments"].getFiniteDouble(segmentsVal)) {
		node->has_segments = true;
		node->segments = static_cast<int>(std::max(segmentsVal, 0.0));
	}

	node->twist = 0.0;
	parameters["twist"].getFiniteDouble(node->twist);
	if (node->twist != 0.0) {
		node->has_twist = true;
	}

	if (node->filename.empty()) {
		children.instantiate(node);
	} else if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
			"module %1$s() does not support child modules when importing a file",inst->name());
	}

	return node;
}

std::string LinearExtrudeNode::toString() const
{
	std::ostringstream stream;

	stream << this->name() << "(";
	if (!this->filename.empty()) { // Ignore deprecated parameters if empty
		fs::path path((std::string)this->filename);
		stream <<
			"file = " << this->filename << ", "
			"layer = " << QuotedString(this->layername) << ", "
			"origin = [" << this->origin_x << ", " << this->origin_y << "], "
			<< "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0) << ", "
			;
	}
	stream << "height = " << std::dec << this->height;
	if (this->center)	{
		stream << ", center = true";
	}
	if (this->has_twist) {
		stream << ", twist = " << this->twist;
	}
	if (this->has_slices) {
		stream << ", slices = " << this->slices;
	}
	if (this->has_segments) {
		stream << ", segments = " << this->segments;
	}

	if (this->scale_x != this->scale_y) {
		stream << ", scale = [" << this->scale_x << ", " << this->scale_y << "]";
	} else if (this->scale_x != 1.0) {
		stream << ", scale = " << this->scale_x ;
	}

	if (!(this->has_slices && this->has_segments)) {
		stream << ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs;
	}
	if (this->convexity > 1) {
		stream << ", convexity = " << this->convexity;
	}
	stream << ")";
	return stream.str();
}

void register_builtin_dxf_linear_extrude()
{
	Builtins::init("dxf_linear_extrude", new BuiltinModule(builtin_linear_extrude));

	Builtins::init("linear_extrude", new BuiltinModule(builtin_linear_extrude),
				{
					"linear_extrude(height = 100, center = false, convexity = 1, twist = 0, scale = 1.0, [slices, segments, $fn, $fs, $fa])",
				});
}
