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

#include "cgaladvnode.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "builtin.h"
#include "children.h"
#include "parameters.h"
#include "polyset.h"
#include <sstream>
#include <assert.h>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

static AbstractNode* builtin_minkowski(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new CgaladvNode(inst, CgaladvType::MINKOWSKI);
	
	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"convexity"});
	node->convexity = static_cast<int>(parameters["convexity"].toDouble());

	return children.instantiate(node);
}

static AbstractNode* builtin_hull(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new CgaladvNode(inst, CgaladvType::HULL);

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
	node->convexity = 0;

	return children.instantiate(node);
}

static AbstractNode* builtin_resize(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new CgaladvNode(inst, CgaladvType::RESIZE);

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"newsize", "auto", "convexity"});
	node->convexity = static_cast<int>(parameters["convexity"].toDouble());
	node->newsize << 0,0,0;
	if ( parameters["newsize"].type() == Value::Type::VECTOR ) {
		const auto &vs = parameters["newsize"].toVector();
		if ( vs.size() >= 1 ) node->newsize[0] = vs[0].toDouble();
		if ( vs.size() >= 2 ) node->newsize[1] = vs[1].toDouble();
		if ( vs.size() >= 3 ) node->newsize[2] = vs[2].toDouble();
	}
	const auto &autosize = parameters["auto"];
	node->autosize << false, false, false;
	if (autosize.type() == Value::Type::VECTOR) {
		const auto &va = autosize.toVector();
		if ( va.size() >= 1 ) node->autosize[0] = va[0].toBool();
		if ( va.size() >= 2 ) node->autosize[1] = va[1].toBool();
		if ( va.size() >= 3 ) node->autosize[2] = va[2].toBool();
	} else if ( autosize.type() == Value::Type::BOOL ) {
		node->autosize << autosize.toBool(),autosize.toBool(),autosize.toBool();
	}

	return children.instantiate(node);
}

std::string CgaladvNode::name() const
{
	switch (this->type) {
	case CgaladvType::MINKOWSKI:
		return "minkowski";
		break;
	case CgaladvType::HULL:
		return "hull";
		break;
	case CgaladvType::RESIZE:
		return "resize";
		break;
	default:
		assert(false);
	}
	return "internal_error";
}

std::string CgaladvNode::toString() const
{
	std::ostringstream stream;

	stream << this->name();
	switch (type) {
	case CgaladvType::MINKOWSKI:
		stream << "(convexity = " << this->convexity << ")";
		break;
	case CgaladvType::HULL:
		stream << "()";
		break;
	case CgaladvType::RESIZE:
		stream << "(newsize = ["
		  << this->newsize[0] << "," << this->newsize[1] << "," << this->newsize[2] << "]"
		  << ", auto = ["
		  << this->autosize[0] << "," << this->autosize[1] << "," << this->autosize[2] << "]"
		  << ", convexity = " << this->convexity
		  << ")";
		break;
	default:
		assert(false);
	}

	return stream.str();
}

void register_builtin_cgaladv()
{
	Builtins::init("minkowski", new BuiltinModule(builtin_minkowski),
				{
					"minkowski(convexity = number)",
				});

	Builtins::init("hull", new BuiltinModule(builtin_hull),
				{
					"hull()",
				});

	Builtins::init("resize", new BuiltinModule(builtin_resize),
				{
					"resize([x, y, z])",
					"resize([x, y, z], boolean)",
					"resize([x, y, z], [boolean, boolean, boolean])",
					"resize([x, y, z], [boolean, boolean, boolean], convexity = number)",
				});
}
