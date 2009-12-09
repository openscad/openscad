/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"
#include "printutils.h"

#include <QFile>

class SurfaceModule : public AbstractModule
{
public:
	SurfaceModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstanciation *inst) const;
};

class SurfaceNode : public AbstractPolyNode
{
public:
	QString filename;
	bool center;
	int convexity;
	SurfaceNode(const ModuleInstanciation *mi) : AbstractPolyNode(mi) { }
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *SurfaceModule::evaluate(const Context *ctx, const ModuleInstanciation *inst) const
{
	SurfaceNode *node = new SurfaceNode(inst);
	node->center = false;
	node->convexity = 1;

	QVector<QString> argnames = QVector<QString>() << "file" << "center" << "convexity";
	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->filename = c.lookup_variable("file").text;

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

void register_builtin_surface()
{
	builtin_modules["surface"] = new SurfaceModule();
}

PolySet *SurfaceNode::render_polyset(render_mode_e) const
{
	handle_dep(filename);
	QFile f(filename);

	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		PRINTF("WARNING: Can't open DXF file `%s'.", filename.toAscii().data());
		return NULL;
	}

	int lines = 0, columns = 0;
	QHash<QPair<int,int>,double> data;
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
			data[QPair<int,int>(lines, i)] = v;
			min_val = fmin(v-1, min_val);
		}
		lines++;
	}

	PolySet *p = new PolySet();
	p->convexity = convexity;

	double ox = center ? -columns/2.0 : 0;
	double oy = center ? -lines/2.0 : 0;

	for (int i = 1; i < lines; i++)
	for (int j = 1; j < columns; j++)
	{
		double v1 = data[QPair<int,int>(i-1, j-1)];
		double v2 = data[QPair<int,int>(i-1, j)];
		double v3 = data[QPair<int,int>(i, j-1)];
		double v4 = data[QPair<int,int>(i, j)];
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
		p->append_vertex(ox + 0, oy + i-1, data[QPair<int,int>(i-1, 0)]);
		p->append_vertex(ox + 0, oy + i, data[QPair<int,int>(i, 0)]);
		p->append_vertex(ox + 0, oy + i, min_val);

		p->append_poly();
		p->insert_vertex(ox + columns-1, oy + i-1, min_val);
		p->insert_vertex(ox + columns-1, oy + i-1, data[QPair<int,int>(i-1, columns-1)]);
		p->insert_vertex(ox + columns-1, oy + i, data[QPair<int,int>(i, columns-1)]);
		p->insert_vertex(ox + columns-1, oy + i, min_val);
	}

	for (int i = 1; i < columns; i++)
	{
		p->append_poly();
		p->insert_vertex(ox + i-1, oy + 0, min_val);
		p->insert_vertex(ox + i-1, oy + 0, data[QPair<int,int>(0, i-1)]);
		p->insert_vertex(ox + i, oy + 0, data[QPair<int,int>(0, i)]);
		p->insert_vertex(ox + i, oy + 0, min_val);

		p->append_poly();
		p->append_vertex(ox + i-1, oy + lines-1, min_val);
		p->append_vertex(ox + i-1, oy + lines-1, data[QPair<int,int>(lines-1, i-1)]);
		p->append_vertex(ox + i, oy + lines-1, data[QPair<int,int>(lines-1, i)]);
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

QString SurfaceNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		text.sprintf("surface(file = \"%s\", center = %s);\n",
				filename.toAscii().data(), center ? "true" : "false");
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}

