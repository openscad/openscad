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

#include <QFile>

enum import_type_e {
	TYPE_STL,
	TYPE_OFF
};

class ImportModule : public AbstractModule
{
public:
	import_type_e type;
	ImportModule(import_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstanciation *inst) const;
};

class ImportNode : public AbstractPolyNode
{
public:
	import_type_e type;
	QString filename;
	int convexity;
	ImportNode(const ModuleInstanciation *mi, import_type_e type) : AbstractPolyNode(mi), type(type) { }
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *ImportModule::evaluate(const Context *ctx, const ModuleInstanciation *inst) const
{
	ImportNode *node = new ImportNode(inst, type);

	QVector<QString> argnames = QVector<QString>() << "filename" << "convexity";
	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->filename = c.lookup_variable("filename").text;
	node->convexity = c.lookup_variable("convexity").num;

	return node;
}

void register_builtin_import()
{
	builtin_modules["import_stl"] = new ImportModule(TYPE_STL);
	builtin_modules["import_off"] = new ImportModule(TYPE_OFF);
}

PolySet *ImportNode::render_polyset(render_mode_e) const
{
	PolySet *p = new PolySet();
	p->convexity = convexity;

	if (type == TYPE_STL)
	{
		handle_dep(filename);
		QFile f(filename);
		if (!f.open(QIODevice::ReadOnly)) {
			PRINTF("WARNING: Can't open import file `%s'.", filename.toAscii().data());
			return p;
		}

		QByteArray data = f.read(5);
		if (data.size() == 5 && QString(data) == QString("solid"))
		{
			int i = 0;
			double vdata[3][3];
			QRegExp splitre = QRegExp("\\s*(vertex)?\\s+");
			f.readLine();
			while (!f.atEnd())
			{
				QString line = QString(f.readLine()).remove("\n").remove("\r");
				if (line.contains("solid") || line.contains("facet") || line.contains("endloop"))
					continue;
				if (line.contains("outer loop")) {
					i = 0;
					continue;
				}
				if (line.contains("vertex")) {
					QStringList tokens = line.split(splitre);
					bool ok[3] = { false, false, false };
					if (tokens.size() == 4) {
						vdata[i][0] = tokens[1].toDouble(&ok[0]);
						vdata[i][1] = tokens[2].toDouble(&ok[1]);
						vdata[i][2] = tokens[3].toDouble(&ok[2]);
					}
					if (!ok[0] || !ok[1] || !ok[2]) {
						PRINTF("WARNING: Can't parse vertex line `%s'.", line.toAscii().data());
						i = 10;
					} else if (++i == 3) {
						p->append_poly();
						p->append_vertex(vdata[0][0], vdata[0][1], vdata[0][2]);
						p->append_vertex(vdata[1][0], vdata[1][1], vdata[1][2]);
						p->append_vertex(vdata[2][0], vdata[2][1], vdata[2][2]);
					}
				}
			}
		}
		else
		{
			f.read(80-4+4);
			while (1) {
				struct {
					float i, j, k;
					float x1, y1, z1;
					float x2, y2, z2;
					float x3, y3, z3;
					unsigned short acount;
				} __attribute__ ((packed)) data;
				if (f.read((char*)&data, sizeof(data)) != sizeof(data))
					break;
				p->append_poly();
				p->append_vertex(data.x1, data.y1, data.z1);
				p->append_vertex(data.x2, data.y2, data.z2);
				p->append_vertex(data.x3, data.y3, data.z3);
			}
		}
	}

	if (type == TYPE_OFF)
	{
		PRINTF("WARNING: OFF import is not implemented yet.");
	}

	return p;
}

QString ImportNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		if (type == TYPE_STL)
			text.sprintf("import_stl(filename = \"%s\");\n", filename.toAscii().data());
		if (type == TYPE_OFF)
			text.sprintf("import_off(filename = \"%s\");\n", filename.toAscii().data());
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}

