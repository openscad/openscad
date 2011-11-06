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
#include "polyset.h"
#include "context.h"
#include "builtin.h"
#include "printutils.h"
#include "handle_dep.h" // handle_dep()
#include "visitor.h"

#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <sstream>
#include <boost/unordered_map.hpp>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class SurfaceModule : public AbstractModule
{
public:
	SurfaceModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class SurfaceNode : public AbstractPolyNode
{
public:
	SurfaceNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "surface"; }

	Filename filename;
	bool center;
	int convexity;
	virtual PolySet *evaluate_polyset(class PolySetEvaluator *) const;
};

AbstractNode *SurfaceModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	SurfaceNode *node = new SurfaceNode(inst);
	node->center = false;
	node->convexity = 1;

	std::vector<std::string> argnames;
	argnames += "file", "center", "convexity";
	std::vector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->filename = c.getAbsolutePath(c.lookup_variable("file").text);

	Value center = c.lookup_variable("center", true);
	if (center.type == Value::BOOL) {
		node->center = center.b;
	}

	Value convexity = c.lookup_variable("convexity", true);
	if (convexity.type == Value::NUMBER) {
		node->convexity = (int)convexity.num;
	}

	return node;
}

PolySet *SurfaceNode::evaluate_polyset(class PolySetEvaluator *) const
{
	handle_dep(filename);
	QFile f(QString::fromStdString(filename));

	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		PRINTF("WARNING: Can't open DAT file `%s'.", filename.c_str());
		return NULL;
	}

	PolySet *p = new PolySet();
	int lines = 0, columns = 0;
	boost::unordered_map<std::pair<int,int>,double> data;
	double min_val = 0;

	while (!f.atEnd())
	{
		QString line = QString(f.readLine()).remove("\n").remove("\r");
		line.replace(QRegExp("^[ \t]+"), "");
		line.replace(QRegExp("[ \t]+$"), "");

		if (line.startsWith("#"))
			continue;

		QStringList fields = line.split(QRegExp("[ \t]+"));
		for (int i = 0; i < fields.count(); i++) {
			if (i >= columns)
				columns = i + 1;
			double v = fields[i].toDouble();
			data[std::make_pair(lines, i)] = v;
			min_val = fmin(v-1, min_val);
		}
		lines++;
	}

	p->convexity = convexity;

	double ox = center ? -columns/2.0 : 0;
	double oy = center ? -lines/2.0 : 0;

	for (int i = 1; i < lines; i++)
	for (int j = 1; j < columns; j++)
	{
		double v1 = data[std::make_pair(i-1, j-1)];
		double v2 = data[std::make_pair(i-1, j)];
		double v3 = data[std::make_pair(i, j-1)];
		double v4 = data[std::make_pair(i, j)];
		double vx = (v1 + v2 + v3 + v4) / 4;

		p->append_poly();
		p->append_vertex(ox + j-1, oy + i-1, v1);
		p->append_vertex(ox + j, oy + i-1, v2);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);

		p->append_poly();
		p->append_vertex(ox + j, oy + i-1, v2);
		p->append_vertex(ox + j, oy + i, v4);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);

		p->append_poly();
		p->append_vertex(ox + j, oy + i, v4);
		p->append_vertex(ox + j-1, oy + i, v3);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);

		p->append_poly();
		p->append_vertex(ox + j-1, oy + i, v3);
		p->append_vertex(ox + j-1, oy + i-1, v1);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);
	}

	for (int i = 1; i < lines; i++)
	{
		p->append_poly();
		p->append_vertex(ox + 0, oy + i-1, min_val);
		p->append_vertex(ox + 0, oy + i-1, data[std::make_pair(i-1, 0)]);
		p->append_vertex(ox + 0, oy + i, data[std::make_pair(i, 0)]);
		p->append_vertex(ox + 0, oy + i, min_val);

		p->append_poly();
		p->insert_vertex(ox + columns-1, oy + i-1, min_val);
		p->insert_vertex(ox + columns-1, oy + i-1, data[std::make_pair(i-1, columns-1)]);
		p->insert_vertex(ox + columns-1, oy + i, data[std::make_pair(i, columns-1)]);
		p->insert_vertex(ox + columns-1, oy + i, min_val);
	}

	for (int i = 1; i < columns; i++)
	{
		p->append_poly();
		p->insert_vertex(ox + i-1, oy + 0, min_val);
		p->insert_vertex(ox + i-1, oy + 0, data[std::make_pair(0, i-1)]);
		p->insert_vertex(ox + i, oy + 0, data[std::make_pair(0, i)]);
		p->insert_vertex(ox + i, oy + 0, min_val);

		p->append_poly();
		p->append_vertex(ox + i-1, oy + lines-1, min_val);
		p->append_vertex(ox + i-1, oy + lines-1, data[std::make_pair(lines-1, i-1)]);
		p->append_vertex(ox + i, oy + lines-1, data[std::make_pair(lines-1, i)]);
		p->append_vertex(ox + i, oy + lines-1, min_val);
	}

	p->append_poly();
	for (int i = 0; i < columns-1; i++)
		p->insert_vertex(ox + i, oy + 0, min_val);
	for (int i = 0; i < lines-1; i++)
		p->insert_vertex(ox + columns-1, oy + i, min_val);
	for (int i = columns-1; i > 0; i--)
		p->insert_vertex(ox + i, oy + lines-1, min_val);
	for (int i = lines-1; i > 0; i--)
		p->insert_vertex(ox + 0, oy + i, min_val);

	return p;
}

std::string SurfaceNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "(file = \"" << this->filename
				 << "\", center = " << (this->center ? "true" : "false") << ")";

	return stream.str();
}

void register_builtin_surface()
{
	Builtins::init("surface", new SurfaceModule());
}
