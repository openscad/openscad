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

#include "colornode.h"
#include "module.h"
#include "context.h"
#include "builtin.h"
#include "printutils.h"
#include <sstream>
#include <assert.h>
#include <QColor>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class ColorModule : public AbstractModule
{
public:
	ColorModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

using std::string;

AbstractNode *ColorModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	ColorNode *node = new ColorNode(inst);

	node->color[0] = node->color[1] = node->color[2] = -1.0;
	node->color[3] = 1.0;

	std::vector<std::string> argnames;
	std::vector<Expression*> argexpr;

	argnames += "c", "alpha";

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value v = c.lookup_variable("c");
	if (v.type == Value::VECTOR) {
		for (size_t i = 0; i < 4; i++)
			node->color[i] = i < v.vec.size() ? v.vec[i]->num : 1.0;
	} else if (v.type == Value::STRING) {
		std::string colorname = v.text;
		QColor color;
		color.setNamedColor(QString::fromStdString(colorname));
		if (color.isValid()) {
			node->color[0] = color.redF();
			node->color[1] = color.greenF();
			node->color[2] = color.blueF();
		} else {
			PRINTF_NOCACHE("WARNING: Color name \"%s\" unknown. Please see", colorname.c_str());
			PRINTF_NOCACHE("WARNING: http://en.wikipedia.org/wiki/Web_colors");
		}
	}
	Value alpha = c.lookup_variable("alpha");
	if (alpha.type == Value::NUMBER) {
		node->color[3] = alpha.num;
	}

	std::vector<AbstractNode *> evaluatednodes = inst->evaluateChildren();
	node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());

	return node;
}

string ColorNode::toString() const
{
	std::stringstream stream;

	stream << "color([" << this->color[0] << ", " << this->color[1] << ", " << this->color[2] << ", " << this->color[3] << "])";

	return stream.str();
}

string ColorNode::name() const
{
	return "color";
}

void register_builtin_color()
{
	builtin_modules["color"] = new ColorModule();
}
