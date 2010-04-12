/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

#include "importnode.h"

#include "module.h"
#include "polyset.h"
#include "context.h"
#include "builtin.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "printutils.h"
#include "openscad.h" // handle_dep()

#include <QFile>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <assert.h>

class ImportModule : public AbstractModule
{
public:
	import_type_e type;
	ImportModule(import_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *ImportModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	ImportNode *node = new ImportNode(inst, type);

	QVector<QString> argnames;
	if (type == TYPE_DXF) {
		argnames = QVector<QString>() << "file" << "layer" << "convexity" << "origin" << "scale";
	} else {
		argnames = QVector<QString>() << "file" << "convexity";
	}
	QVector<Expression*> argexpr;

	// Map old argnames to new argnames for compatibility
	QVector<QString> inst_argnames = inst->argnames;
	for (int i=0; i<inst_argnames.size(); i++) {
		if (inst_argnames[i] == "filename")
			inst_argnames[i] = "file";
		if (inst_argnames[i] == "layername")
			inst_argnames[i] = "layer";
	}

	Context c(ctx);
	c.args(argnames, argexpr, inst_argnames, inst->argvalues);

	node->fn = c.lookup_variable("$fn").num;
	node->fs = c.lookup_variable("$fs").num;
	node->fa = c.lookup_variable("$fa").num;

	node->filename = c.get_absolute_path(c.lookup_variable("file").text);
	node->layername = c.lookup_variable("layer", true).text;
	node->convexity = c.lookup_variable("convexity", true).num;

	if (node->convexity <= 0)
		node->convexity = 1;

	Value origin = c.lookup_variable("origin", true);
	node->origin_x = node->origin_y = 0;
	origin.getv2(node->origin_x, node->origin_y);

	node->scale = c.lookup_variable("scale", true).num;

	if (node->scale <= 0)
		node->scale = 1;

	return node;
}

void register_builtin_import()
{
	builtin_modules["import_stl"] = new ImportModule(TYPE_STL);
	builtin_modules["import_off"] = new ImportModule(TYPE_OFF);
	builtin_modules["import_dxf"] = new ImportModule(TYPE_DXF);
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
			f.read(80-5+4);
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

	if (type == TYPE_DXF)
	{
		DxfData dd(fn, fs, fa, filename, layername, origin_x, origin_y, scale);
		p->is2d = true;
		dxf_tesselate(p, &dd, 0, true, false, 0);
		dxf_border_to_ps(p, &dd);
	}

	return p;
}

std::string ImportNode::toString() const
{
	std::stringstream stream;

	QString text;
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(this->filename.toAscii().data(), &st);

	switch (this->type) {
	case TYPE_STL:
		stream << "import_stl(file = \"" << this->filename << "\", "
			"cache = \"" << std::hex << (int)st.st_mtime << "." << (int)st.st_size << "\", "
			"convexity = " << std::dec << this->convexity << ")";
		break;
	case TYPE_OFF:
		stream << "import_off(file = \"" << this->filename << "\", "
			"cache = \"" << std::hex << (int)st.st_mtime << "." << (int)st.st_size << "\", "
			"convexity = " << std::dec << this->convexity << ")";
		break;
	case TYPE_DXF:
		stream << "import_dxf(file = \"" << this->filename << "\", "
			"cache = \"" << std::hex << (int)st.st_mtime << "." << (int)st.st_size << "\", "
			"layer = \"" << this->layername << "\", "
			"origin = [ " << std::dec << this->origin_x << " " << this->origin_y << " ], "
			"scale = " << this->scale << ", "
			"convexity = " << this->convexity << ", "
			"$fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs << ")";
		break;
	default:
		assert(false);
	}

	return stream.str();
}
