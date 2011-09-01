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

#include "module.h"
#include "node.h"
#include "context.h"
#include "builtin.h"
#include "printutils.h"
#include "visitor.h"
#include <sstream>
#include <assert.h>

enum cgaladv_type_e {
	MINKOWSKI,
	GLIDE,
	SUBDIV,
	HULL
};

class CgaladvModule : public AbstractModule
{
public:
	cgaladv_type_e type;
	CgaladvModule(cgaladv_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class CgaladvNode : public AbstractNode
{
public:
	CgaladvNode(const ModuleInstantiation *mi, cgaladv_type_e type) : AbstractNode(mi), type(type) {
		convexity = 1;
	}
	virtual ~CgaladvNode() { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const {
		switch (this->type) {
		case MINKOWSKI:
			return "minkowski";
			break;
		case GLIDE:
			return "glide";
			break;
		case SUBDIV:
			return "subdiv";
			break;
		case HULL:
			return "hull";
			break;
		default:
			assert(false);
		}
	}

	Value path;
	std::string subdiv_type;
	int convexity, level;
	cgaladv_type_e type;
};

AbstractNode *CgaladvModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	CgaladvNode *node = new CgaladvNode(inst, type);

	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	if (type == MINKOWSKI)
		argnames = QVector<QString>() << "convexity";

	if (type == GLIDE)
		argnames = QVector<QString>() << "path" << "convexity";

	if (type == SUBDIV)
		argnames = QVector<QString>() << "type" << "level" << "convexity";

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value convexity, path, subdiv_type, level;

	if (type == MINKOWSKI) {
		convexity = c.lookup_variable("convexity", true);
	}

	if (type == GLIDE) {
		convexity = c.lookup_variable("convexity", true);
		path = c.lookup_variable("path", false);
	}

	if (type == SUBDIV) {
		convexity = c.lookup_variable("convexity", true);
		subdiv_type = c.lookup_variable("type", false);
		level = c.lookup_variable("level", true);
	}

	node->convexity = (int)convexity.num;
	node->path = path;
	node->subdiv_type = subdiv_type.text;
	node->level = (int)level.num;

	if (node->level <= 1)
		node->level = 1;

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n)
			node->children.push_back(n);
	}

	return node;
}

void register_builtin_cgaladv()
{
	builtin_modules["minkowski"] = new CgaladvModule(MINKOWSKI);
	builtin_modules["glide"] = new CgaladvModule(GLIDE);
	builtin_modules["subdiv"] = new CgaladvModule(SUBDIV);
	builtin_modules["hull"] = new CgaladvModule(HULL);
}

std::string CgaladvNode::toString() const
{
	std::stringstream stream;

	stream << this->name();
	switch (type) {
	case MINKOWSKI:
		stream << "(convexity = " << this->convexity << ")";
		break;
	case GLIDE:
		stream << "(path = " << this->path << ", convexity = " << this->convexity << ")";
		break;
	case SUBDIV:
		stream << "(level = " << this->level << ", convexity = " << this->convexity << ")";
		break;
	case HULL:
		stream << "()";
		break;
	default:
		assert(false);
	}

	return stream.str();
}
