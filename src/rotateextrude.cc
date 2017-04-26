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

#include "rotateextrudenode.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "polyset.h"

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class RotateExtrudeModule : public AbstractModule
{
public:
	RotateExtrudeModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

AbstractNode *RotateExtrudeModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	auto node = new RotateExtrudeNode(inst);

	AssignmentList args{Assignment("file"), Assignment("layer"), Assignment("origin"), Assignment("scale")};

	Context c(ctx);
	c.setVariables(args, evalctx);
	inst->scope.apply(*evalctx);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();
    

	auto file = c.lookup_variable("file");
	auto layer = c.lookup_variable("layer", true);
	auto convexity = c.lookup_variable("convexity", true);
	auto origin = c.lookup_variable("origin", true);
	auto scale = c.lookup_variable("scale", true);
	auto angle = c.lookup_variable("angle", true);
    
	if (!file->isUndefined()) {
		printDeprecation("Support for reading files in rotate_extrude will be removed in future releases. Use a child import() instead.");
		node->filename = lookup_file(file->toString(), inst->path(), c.documentPath());
	}

	node->layername = layer->isUndefined() ? "" : layer->toString();
	node->convexity = static_cast<int>(convexity->toDouble());
	origin->getVec2(node->origin_x, node->origin_y);
	node->scale = scale->toDouble();
	node->angle = 360;
	angle->getFiniteDouble(node->angle);

	if (node->convexity <= 0)
		node->convexity = 2;

	if (node->scale <= 0)
		node->scale = 1;

	if ((node->angle <= -360) || (node->angle > 360))
		node->angle = 360;

	if (node->filename.empty()) {
		auto instantiatednodes = inst->instantiateChildren(evalctx);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}

	return node;
}

std::string RotateExtrudeNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "(";
	if (!this->filename.empty()) { // Ignore deprecated parameters if empty 
		fs::path path((std::string)this->filename);
		stream <<
			"file = " << this->filename << ", "
			"layer = " << QuotedString(this->layername) << ", "
			"origin = [" << std::dec << this->origin_x << ", " << this->origin_y << "], "
			"scale = " << this->scale << ", "
			<< "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0) << ", "
			;
	}
	stream <<
		"angle = " << this->angle << ", "
		"convexity = " << this->convexity << ", "
		"$fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs << ")";

	return stream.str();
}

void register_builtin_dxf_rotate_extrude()
{
	Builtins::init("dxf_rotate_extrude", new RotateExtrudeModule());
	Builtins::init("rotate_extrude", new RotateExtrudeModule());
}
