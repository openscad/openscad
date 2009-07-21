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

class DxfLinearExtrudeModule : public AbstractModule
{
public:
	DxfLinearExtrudeModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstanciation *inst) const;
};

class DxfLinearExtrudeNode : public AbstractPolyNode
{
public:
	int convexity;
	double fn, fs, fa, height;
	double origin_x, origin_y, scale;
	bool center;
	QString filename, layername;
	DxfLinearExtrudeNode(const ModuleInstanciation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		fn = fs = fa = height = 0;
		origin_x = origin_y = scale = 0;
		center = false;
	}
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *DxfLinearExtrudeModule::evaluate(const Context *ctx, const ModuleInstanciation *inst) const
{
	DxfLinearExtrudeNode *node = new DxfLinearExtrudeNode(inst);

	QVector<QString> argnames = QVector<QString>() << "file" << "layer" << "height" << "origin" << "scale" << "center";
	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->fn = c.lookup_variable("$fn").num;
	node->fs = c.lookup_variable("$fs").num;
	node->fa = c.lookup_variable("$fa").num;

	Value file = c.lookup_variable("file");
	Value layer = c.lookup_variable("layer", true);
	Value height = c.lookup_variable("height", true);
	Value convexity = c.lookup_variable("convexity", true);
	Value origin = c.lookup_variable("origin", true);
	Value scale = c.lookup_variable("scale", true);
	Value center = c.lookup_variable("center", true);

	node->filename = file.text;
	node->layername = layer.text;
	node->height = height.num;
	node->convexity = convexity.num;
	origin.getv2(node->origin_x, node->origin_y);
	node->scale = scale.num;

	if (center.type == Value::BOOL)
		node->center = center.b;

	if (node->height <= 0)
		node->height = 100;

	if (node->convexity <= 0)
		node->convexity = 1;

	if (node->scale <= 0)
		node->scale = 1;

	return node;
}

void register_builtin_dxf_linear_extrude()
{
	builtin_modules["dxf_linear_extrude"] = new DxfLinearExtrudeModule();
}

static void add_slice(PolySet *ps, DxfData::Path *pt, double h1, double h2)
{
	for (int j = 1; j < pt->points.count(); j++)
	{
		int k = j - 1;
		ps->append_poly();
		if (pt->is_inner) {
			ps->append_vertex(pt->points[k]->x, pt->points[k]->y, h1);
			ps->append_vertex(pt->points[j]->x, pt->points[j]->y, h1);
			ps->append_vertex(pt->points[j]->x, pt->points[j]->y, h2);
		} else {
			ps->insert_vertex(pt->points[k]->x, pt->points[k]->y, h1);
			ps->insert_vertex(pt->points[j]->x, pt->points[j]->y, h1);
			ps->insert_vertex(pt->points[j]->x, pt->points[j]->y, h2);
		}

		ps->append_poly();
		if (pt->is_inner) {
			ps->append_vertex(pt->points[k]->x, pt->points[k]->y, h2);
			ps->append_vertex(pt->points[k]->x, pt->points[k]->y, h1);
			ps->append_vertex(pt->points[j]->x, pt->points[j]->y, h2);
		} else {
			ps->insert_vertex(pt->points[k]->x, pt->points[k]->y, h2);
			ps->insert_vertex(pt->points[k]->x, pt->points[k]->y, h1);
			ps->insert_vertex(pt->points[j]->x, pt->points[j]->y, h2);
		}
	}
}

PolySet *DxfLinearExtrudeNode::render_polyset(render_mode_e) const
{
	DxfData dxf(fn, fs, fa, filename, layername, origin_x, origin_y, scale);

	PolySet *ps = new PolySet();
	ps->convexity = convexity;

	double h1, h2;

	if (center) {
		h1 = -height/2.0;
		h2 = -height/2.0;
	} else {
		h1 = 0;
		h2 = height;
	}

	for (int i = 0; i < dxf.paths.count(); i++)
	{
		if (!dxf.paths[i].is_closed)
			continue;
		add_slice(ps, &dxf.paths[i], h1, h2);
	}

	dxf_tesselate(ps, &dxf, false, h1);
	dxf_tesselate(ps, &dxf, true, h2);

	return ps;
}

QString DxfLinearExtrudeNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		text.sprintf("dxf_linear_extrude(file = \"%s\", layer = \"%s\", height = %f, "
				"origin = [ %f %f ], scale = %f, "
				"$fn = %f, $fa = %f, $fs = %f);\n",
				filename.toAscii().data(), layername.toAscii().data(),
				height, origin_x, origin_y, scale, fn, fs, fa);
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}

