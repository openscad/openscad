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

#include "csgnode.h"

#include "module.h"
#include "csgterm.h"
#include "builtin.h"
#include "printutils.h"
#include <sstream>
#include <assert.h>

class CsgModule : public AbstractModule
{
public:
	csg_type_e type;
	CsgModule(csg_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *CsgModule::evaluate(const Context*, const ModuleInstantiation *inst) const
{
	CsgNode *node = new CsgNode(inst, type);
	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n != NULL)
			node->children.push_back(n);
	}
	return node;
}

std::string CsgNode::toString() const
{
	return this->name() + "()";
}

std::string CsgNode::name() const
{
	switch (this->type) {
	case CSG_TYPE_UNION:
		return "union";
		break;
	case CSG_TYPE_DIFFERENCE:
		return "difference";
		break;
	case CSG_TYPE_INTERSECTION:
		return "intersection";
		break;
	default:
		assert(false);
	}
}

void register_builtin_csgops()
{
	builtin_modules["union"] = new CsgModule(CSG_TYPE_UNION);
	builtin_modules["difference"] = new CsgModule(CSG_TYPE_DIFFERENCE);
	builtin_modules["intersection"] = new CsgModule(CSG_TYPE_INTERSECTION);
}

