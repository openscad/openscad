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
#include "dxfdata.h"
#include "dxftess.h"
#include "polyset.h"
#include "progress.h"
#include "openscad.h" // get_fragments_from_r()

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QApplication>
#include <QTime>
#include <QProgressDialog>

class DxfLinearExtrudeModule : public AbstractModule
{
public:
	DxfLinearExtrudeModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class DxfLinearExtrudeNode : public AbstractPolyNode
{
public:
	int convexity, slices;
	double fn, fs, fa, height, twist;
	double origin_x, origin_y, scale;
	bool center, has_twist;
	QString filename, layername;
	DxfLinearExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = slices = 0;
		fn = fs = fa = height = twist = 0;
		origin_x = origin_y = scale = 0;
		center = has_twist = false;
	}
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *DxfLinearExtrudeModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	DxfLinearExtrudeNode *node = new DxfLinearExtrudeNode(inst);

	QVector<QString> argnames = QVector<QString>() << "file" << "layer" << "height" << "origin" << "scale" << "center" << "twist" << "slices";
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
	Value twist = c.lookup_variable("twist", true);
	Value slices = c.lookup_variable("slices", true);

	if(!file.text.isNull())
		node->filename = c.get_absolute_path(file.text);
	else
		node->filename = file.text;

	node->layername = layer.text;
	node->height = height.num;
	node->convexity = (int)convexity.num;
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

	if (twist.type == Value::NUMBER) {
		node->twist = twist.num;
		if (slices.type == Value::NUMBER) {
			node->slices = (int)slices.num;
		} else {
			node->slices = (int)fmax(2, fabs(get_fragments_from_r(node->height,
					node->fn, node->fs, node->fa) * node->twist / 360));
		}
		node->has_twist = true;
	}

	if (node->filename.isEmpty()) {
		foreach (ModuleInstantiation *v, inst->children) {
			AbstractNode *n = v->evaluate(inst->ctx);
			if (n)
				node->children.append(n);
		}
	}

	return node;
}

void register_builtin_dxf_linear_extrude()
{
	builtin_modules["dxf_linear_extrude"] = new DxfLinearExtrudeModule();
	builtin_modules["linear_extrude"] = new DxfLinearExtrudeModule();
}

static void add_slice(PolySet *ps, DxfData::Path *pt, double rot1, double rot2, double h1, double h2)
{
	for (int j = 1; j < pt->points.count(); j++)
	{
		int k = j - 1;

		double jx1 = pt->points[j]->x *  cos(rot1*M_PI/180) + pt->points[j]->y * sin(rot1*M_PI/180);
		double jy1 = pt->points[j]->x * -sin(rot1*M_PI/180) + pt->points[j]->y * cos(rot1*M_PI/180);

		double jx2 = pt->points[j]->x *  cos(rot2*M_PI/180) + pt->points[j]->y * sin(rot2*M_PI/180);
		double jy2 = pt->points[j]->x * -sin(rot2*M_PI/180) + pt->points[j]->y * cos(rot2*M_PI/180);

		double kx1 = pt->points[k]->x *  cos(rot1*M_PI/180) + pt->points[k]->y * sin(rot1*M_PI/180);
		double ky1 = pt->points[k]->x * -sin(rot1*M_PI/180) + pt->points[k]->y * cos(rot1*M_PI/180);

		double kx2 = pt->points[k]->x *  cos(rot2*M_PI/180) + pt->points[k]->y * sin(rot2*M_PI/180);
		double ky2 = pt->points[k]->x * -sin(rot2*M_PI/180) + pt->points[k]->y * cos(rot2*M_PI/180);

		double dia1_len_sq = (jy1-ky2)*(jy1-ky2) + (jx1-kx2)*(jx1-kx2);
		double dia2_len_sq = (jy2-ky1)*(jy2-ky1) + (jx2-kx1)*(jx2-kx1);

		if (dia1_len_sq > dia2_len_sq)
		{
			ps->append_poly();
			if (pt->is_inner) {
				ps->append_vertex(kx1, ky1, h1);
				ps->append_vertex(jx1, jy1, h1);
				ps->append_vertex(jx2, jy2, h2);
			} else {
				ps->insert_vertex(kx1, ky1, h1);
				ps->insert_vertex(jx1, jy1, h1);
				ps->insert_vertex(jx2, jy2, h2);
			}

			ps->append_poly();
			if (pt->is_inner) {
				ps->append_vertex(kx2, ky2, h2);
				ps->append_vertex(kx1, ky1, h1);
				ps->append_vertex(jx2, jy2, h2);
			} else {
				ps->insert_vertex(kx2, ky2, h2);
				ps->insert_vertex(kx1, ky1, h1);
				ps->insert_vertex(jx2, jy2, h2);
			}
		}
		else
		{
			ps->append_poly();
			if (pt->is_inner) {
				ps->append_vertex(kx1, ky1, h1);
				ps->append_vertex(jx1, jy1, h1);
				ps->append_vertex(kx2, ky2, h2);
			} else {
				ps->insert_vertex(kx1, ky1, h1);
				ps->insert_vertex(jx1, jy1, h1);
				ps->insert_vertex(kx2, ky2, h2);
			}

			ps->append_poly();
			if (pt->is_inner) {
				ps->append_vertex(jx2, jy2, h2);
				ps->append_vertex(kx2, ky2, h2);
				ps->append_vertex(jx1, jy1, h1);
			} else {
				ps->insert_vertex(jx2, jy2, h2);
				ps->insert_vertex(kx2, ky2, h2);
				ps->insert_vertex(jx1, jy1, h1);
			}
		}
	}
}

PolySet *DxfLinearExtrudeNode::render_polyset(render_mode_e) const
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

		// Before extruding, union all (2D) children nodes
		// to a single DxfData, then tesselate this into a PolySet
		CGAL_Nef_polyhedron N;
		N.dim = 2;
		foreach(AbstractNode * v, children) {
			if (v->modinst->tag_background)
				continue;
			N.p2 += v->render_cgal_nef_polyhedron().p2;
		}
		dxf = new DxfData(N);

#else // ENABLE_CGAL
		PRINT("WARNING: Found linear_extrude() statement without dxf file but compiled without CGAL support!");
		dxf = new DxfData();
#endif // ENABLE_CGAL
	} else {
		dxf = new DxfData(fn, fs, fa, filename, layername, origin_x, origin_y, scale);
	}

	PolySet *ps = new PolySet();
	ps->convexity = convexity;

	double h1, h2;

	if (center) {
		h1 = -height/2.0;
		h2 = +height/2.0;
	} else {
		h1 = 0;
		h2 = height;
	}

	bool first_open_path = true;
	for (int i = 0; i < dxf->paths.count(); i++)
	{
		if (dxf->paths[i].is_closed)
			continue;
		if (first_open_path) {
			PRINTF("WARING: Open paths in dxf_liniear_extrude(file = \"%s\", layer = \"%s\"):",
					filename.toAscii().data(), layername.toAscii().data());
			first_open_path = false;
		}
		PRINTF("   %9.5f %10.5f ... %10.5f %10.5f",
				dxf->paths[i].points.first()->x / scale + origin_x,
				dxf->paths[i].points.first()->y / scale + origin_y, 
				dxf->paths[i].points.last()->x / scale + origin_x,
				dxf->paths[i].points.last()->y / scale + origin_y);
	}


	if (has_twist)
	{
		dxf_tesselate(ps, dxf, 0, false, true, h1);
		dxf_tesselate(ps, dxf, twist, true, true, h2);
		for (int j = 0; j < slices; j++)
		{
			double t1 = twist*j / slices;
			double t2 = twist*(j+1) / slices;
			double g1 = h1 + (h2-h1)*j / slices;
			double g2 = h1 + (h2-h1)*(j+1) / slices;
			for (int i = 0; i < dxf->paths.count(); i++)
			{
				if (!dxf->paths[i].is_closed)
					continue;
				add_slice(ps, &dxf->paths[i], t1, t2, g1, g2);
			}
		}
	}
	else
	{
		dxf_tesselate(ps, dxf, 0, false, true, h1);
		dxf_tesselate(ps, dxf, 0, true, true, h2);
		for (int i = 0; i < dxf->paths.count(); i++)
		{
			if (!dxf->paths[i].is_closed)
				continue;
			add_slice(ps, &dxf->paths[i], 0, 0, h1, h2);
		}
	}

	PolySet::ps_cache.insert(key, new PolySet::ps_cache_entry(ps->link()));
	print_messages_pop();
	delete dxf;

	return ps;
}

QString DxfLinearExtrudeNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		struct stat st;
		memset(&st, 0, sizeof(struct stat));
		stat(filename.toAscii().data(), &st);
		text.sprintf("linear_extrude(file = \"%s\", cache = \"%x.%x\", layer = \"%s\", "
				"height = %g, origin = [ %g %g ], scale = %g, center = %s, convexity = %d",
				filename.toAscii().data(), (int)st.st_mtime, (int)st.st_size,
				layername.toAscii().data(), height, origin_x, origin_y, scale,
				center ? "true" : "false", convexity);
		if (has_twist) {
			QString t2;
			t2.sprintf(", twist = %g, slices = %d", twist, slices);
			text += t2;
		}
		QString t3;
		t3.sprintf(", $fn = %g, $fa = %g, $fs = %g) {\n", fn, fa, fs);
		text += t3;
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		text += indent + "}\n";
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}

