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

#include "module.h"
#include "node.h"
#include "context.h"
#include "printutils.h"
#include "builtin.h"
#include "polyset.h"
#include "dxfdata.h"
#include "progress.h"
#include "openscad.h" // get_fragments_from_r()

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QTime>
#include <QApplication>
#include <QProgressDialog>

class DxfRotateExtrudeModule : public AbstractModule
{
public:
	DxfRotateExtrudeModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class DxfRotateExtrudeNode : public AbstractPolyNode
{
public:
	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	QString filename, layername;
	DxfRotateExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		fn = fs = fa = 0;
		origin_x = origin_y = scale = 0;
	}
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *DxfRotateExtrudeModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	DxfRotateExtrudeNode *node = new DxfRotateExtrudeNode(inst);

	QVector<QString> argnames = QVector<QString>() << "file" << "layer" << "origin" << "scale";
	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->fn = c.lookup_variable("$fn").num;
	node->fs = c.lookup_variable("$fs").num;
	node->fa = c.lookup_variable("$fa").num;

	Value file = c.lookup_variable("file");
	Value layer = c.lookup_variable("layer", true);
	Value convexity = c.lookup_variable("convexity", true);
	Value origin = c.lookup_variable("origin", true);
	Value scale = c.lookup_variable("scale", true);

	node->filename = c.get_absolute_path(file.text);
	node->layername = layer.text;
	node->convexity = (int)convexity.num;
	origin.getv2(node->origin_x, node->origin_y);
	node->scale = scale.num;

	if (node->convexity <= 0)
		node->convexity = 1;

	if (node->scale <= 0)
		node->scale = 1;

	if (node->filename.isEmpty()) {
		foreach (ModuleInstantiation *v, inst->children) {
			AbstractNode *n = v->evaluate(inst->ctx);
			if (n)
				node->children.append(n);
		}
	}

	return node;
}

void register_builtin_dxf_rotate_extrude()
{
	builtin_modules["dxf_rotate_extrude"] = new DxfRotateExtrudeModule();
	builtin_modules["rotate_extrude"] = new DxfRotateExtrudeModule();
}

PolySet *DxfRotateExtrudeNode::render_polyset(render_mode_e) const
{
	QString key = mk_cache_id();
	if (PolySet::ps_cache.contains(key)) {
		PRINT(PolySet::ps_cache[key]->msg);
		return PolySet::ps_cache[key]->ps->link();
	}

	print_messages_push();
	DxfData *dxf;

	if (filename.isEmpty())
	{
#ifdef ENABLE_CGAL
		CGAL_Nef_polyhedron N;
		N.dim = 2;
		foreach(AbstractNode * v, children) {
			if (v->modinst->tag_background)
				continue;
			N.p2 += v->render_cgal_nef_polyhedron().p2;
		}
		dxf = new DxfData(N);

#else // ENABLE_CGAL
		PRINT("WARNING: Found rotate_extrude() statement without dxf file but compiled without CGAL support!");
		dxf = new DxfData();
#endif // ENABLE_CGAL
	} else {
		dxf = new DxfData(fn, fs, fa, filename, layername, origin_x, origin_y, scale);
	}

	PolySet *ps = new PolySet();
	ps->convexity = convexity;

	for (int i = 0; i < dxf->paths.count(); i++)
	{
		double max_x = 0;
		for (int j = 0; j < dxf->paths[i].points.count(); j++) {
			max_x = fmax(max_x, dxf->paths[i].points[j]->x);
		}

		int fragments = get_fragments_from_r(max_x, fn, fs, fa);

		double points[fragments][dxf->paths[i].points.count()][3];

		for (int j = 0; j < fragments; j++) {
			double a = (j*2*M_PI) / fragments;
			for (int k = 0; k < dxf->paths[i].points.count(); k++) {
				if (dxf->paths[i].points[k]->x == 0) {
					points[j][k][0] = 0;
					points[j][k][1] = 0;
				} else {
					points[j][k][0] = dxf->paths[i].points[k]->x * sin(a);
					points[j][k][1] = dxf->paths[i].points[k]->x * cos(a);
				}
				points[j][k][2] = dxf->paths[i].points[k]->y;
			}
		}

		for (int j = 0; j < fragments; j++) {
			int j1 = j + 1 < fragments ? j + 1 : 0;
			for (int k = 0; k < dxf->paths[i].points.count(); k++) {
				int k1 = k + 1 < dxf->paths[i].points.count() ? k + 1 : 0;
				if (points[j][k][0] != points[j1][k][0] ||
						points[j][k][1] != points[j1][k][1] ||
						points[j][k][2] != points[j1][k][2]) {
					ps->append_poly();
					ps->append_vertex(points[j ][k ][0],
							points[j ][k ][1], points[j ][k ][2]);
					ps->append_vertex(points[j1][k ][0],
							points[j1][k ][1], points[j1][k ][2]);
					ps->append_vertex(points[j ][k1][0],
							points[j ][k1][1], points[j ][k1][2]);
				}
				if (points[j][k1][0] != points[j1][k1][0] ||
						points[j][k1][1] != points[j1][k1][1] ||
						points[j][k1][2] != points[j1][k1][2]) {
					ps->append_poly();
					ps->append_vertex(points[j ][k1][0],
							points[j ][k1][1], points[j ][k1][2]);
					ps->append_vertex(points[j1][k ][0],
							points[j1][k ][1], points[j1][k ][2]);
					ps->append_vertex(points[j1][k1][0],
							points[j1][k1][1], points[j1][k1][2]);
				}
			}
		}
	}

	PolySet::ps_cache.insert(key, new PolySet::ps_cache_entry(ps->link()));
	print_messages_pop();
	delete dxf;

	return ps;
}

QString DxfRotateExtrudeNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		struct stat st;
		memset(&st, 0, sizeof(struct stat));
		stat(filename.toAscii().data(), &st);
		text.sprintf("rotate_extrude(file = \"%s\", cache = \"%x.%x\", layer = \"%s\", "
				"origin = [ %g %g ], scale = %g, convexity = %d, "
				"$fn = %g, $fa = %g, $fs = %g) {\n",
				filename.toAscii().data(), (int)st.st_mtime, (int)st.st_size,
				layername.toAscii().data(), origin_x, origin_y, scale, convexity,
				fn, fs, fa);
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		text += indent + "}\n";
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}

