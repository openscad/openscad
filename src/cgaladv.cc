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
#include "evalcontext.h"
#include "builtin.h"
#include "polyset.h"
#include <sstream>
#include <assert.h>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class CgaladvModule : public AbstractModule
{
public:
	CgaladvType type;
	CgaladvModule(CgaladvType type) : type(type) { }
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *CgaladvModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new CgaladvNode(inst, evalctx, type);

	AssignmentList args;

	if (type == CgaladvType::MINKOWSKI) {
		args += assignment("convexity");
	}

	if (type == CgaladvType::RESIZE) {
		args += assignment("newsize"), assignment("auto"), assignment("convexity");
	}

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args);
	inst->scope.apply(evalctx);

	if (type == CgaladvType::MINKOWSKI) {
		const auto &convexity = c->lookup_variable("convexity", true);
		node->convexity = static_cast<int>(convexity.toDouble());
	} else if (type == CgaladvType::RESIZE) {
		const auto &convexity = c->lookup_variable("convexity", true);
		node->convexity = static_cast<int>(convexity.toDouble());
		const auto &ns = c->lookup_variable("newsize");
		node->newsize << 0,0,0;
		if ( ns.type() == Value::Type::VECTOR ) {
			const auto &vs = ns.toVector();
			if ( vs.size() >= 1 ) node->newsize[0] = vs[0].toDouble();
			if ( vs.size() >= 2 ) node->newsize[1] = vs[1].toDouble();
			if ( vs.size() >= 3 ) node->newsize[2] = vs[2].toDouble();
		}
		const auto &autosize = c->lookup_variable("auto");
		node->autosize << false, false, false;
		if (autosize.type() == Value::Type::VECTOR) {
			const auto &va = autosize.toVector();
			if ( va.size() >= 1 ) node->autosize[0] = va[0].toBool();
			if ( va.size() >= 2 ) node->autosize[1] = va[1].toBool();
			if ( va.size() >= 3 ) node->autosize[2] = va[2].toBool();
		}
		else if ( autosize.type() == Value::Type::BOOL ) {
			node->autosize << autosize.toBool(),autosize.toBool(),autosize.toBool();
		}
	} else {
		node->convexity = 0;
	}

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
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
	Builtins::init("minkowski", new CgaladvModule(CgaladvType::MINKOWSKI),
				{
					"minkowski(convexity = number)",
				});

	Builtins::init("hull", new CgaladvModule(CgaladvType::HULL),
				{
					"hull()",
				});

	Builtins::init("resize", new CgaladvModule(CgaladvType::RESIZE),
				{
					"resize([x, y, z])",
					"resize([x, y, z], boolean)",
					"resize([x, y, z], [boolean, boolean, boolean])",
					"resize([x, y, z], [boolean, boolean, boolean], convexity = number)",
				});
}
