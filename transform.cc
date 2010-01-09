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

enum transform_type_e {
	SCALE,
	ROTATE,
	MIRROR,
	TRANSLATE,
	MULTMATRIX,
	COLOR
};

class TransformModule : public AbstractModule
{
public:
	transform_type_e type;
	TransformModule(transform_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class TransformNode : public AbstractNode
{
public:
	double m[20];
	TransformNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *TransformModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	TransformNode *node = new TransformNode(inst);

	for (int i = 0; i < 16; i++)
		node->m[i] = i % 5 == 0 ? 1.0 : 0.0;
	for (int i = 16; i < 20; i++)
		node->m[i] = -1;

	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	if (type == SCALE) {
		argnames = QVector<QString>() << "v";
	}
	if (type == ROTATE) {
		argnames = QVector<QString>() << "a" << "v";
	}
	if (type == MIRROR) {
		argnames = QVector<QString>() << "v";
	}
	if (type == TRANSLATE) {
		argnames = QVector<QString>() << "v";
	}
	if (type == MULTMATRIX) {
		argnames = QVector<QString>() << "m";
	}
	if (type == COLOR) {
		argnames = QVector<QString>() << "c";
	}

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	if (type == SCALE)
	{
		Value v = c.lookup_variable("v");
		v.getnum(node->m[0]);
		v.getnum(node->m[5]);
		v.getnum(node->m[10]);
		v.getv3(node->m[0], node->m[5], node->m[10]);
		if (node->m[10] <= 0)
			node->m[10] = 1;
	}
	if (type == ROTATE)
	{
		Value val_a = c.lookup_variable("a");
		if (val_a.type == Value::VECTOR)
		{
			for (int i = 0; i < 3; i++) {
				double a;
				val_a.vec[i]->getnum(a);
				double c = cos(a*M_PI/180.0);
				double s = sin(a*M_PI/180.0);
				double x = i == 0, y = i == 1, z = i == 2;
				double mr[16] = {
					x*x*(1-c)+c,
					y*x*(1-c)+z*s,
					z*x*(1-c)-y*s,
					0,
					x*y*(1-c)-z*s,
					y*y*(1-c)+c,
					z*y*(1-c)+x*s,
					0,
					x*z*(1-c)+y*s,
					y*z*(1-c)-x*s,
					z*z*(1-c)+c,
					0,
					0, 0, 0, 1
				};
				double m[16];
				for (int x = 0; x < 4; x++)
				for (int y = 0; y < 4; y++)
				{
					m[x+y*4] = 0;
					for (int i = 0; i < 4; i++)
						m[x+y*4] += node->m[i+y*4] * mr[x+i*4];
				}
				for (int i = 0; i < 16; i++)
					node->m[i] = m[i];
			}
		}
		else
		{
			Value val_v = c.lookup_variable("v");
			double a = 0, x = 0, y = 0, z = 1;

			val_a.getnum(a);

			if (val_v.getv3(x, y, z)) {
				if (x != 0.0 || y != 0.0 || z != 0.0) {
					double sn = 1.0 / sqrt(x*x + y*y + z*z);
					x *= sn, y *= sn, z *= sn;
				}
			}

			if (x != 0.0 || y != 0.0 || z != 0.0)
			{
				double c = cos(a*M_PI/180.0);
				double s = sin(a*M_PI/180.0);

				node->m[ 0] = x*x*(1-c)+c;
				node->m[ 1] = y*x*(1-c)+z*s;
				node->m[ 2] = z*x*(1-c)-y*s;

				node->m[ 4] = x*y*(1-c)-z*s;
				node->m[ 5] = y*y*(1-c)+c;
				node->m[ 6] = z*y*(1-c)+x*s;

				node->m[ 8] = x*z*(1-c)+y*s;
				node->m[ 9] = y*z*(1-c)-x*s;
				node->m[10] = z*z*(1-c)+c;
			}
		}
	}
	if (type == MIRROR)
	{
		Value val_v = c.lookup_variable("v");
		double x = 1, y = 0, z = 0;
	
		if (val_v.getv3(x, y, z)) {
			if (x != 0.0 || y != 0.0 || z != 0.0) {
				double sn = 1.0 / sqrt(x*x + y*y + z*z);
				x *= sn, y *= sn, z *= sn;
			}
		}

		if (x != 0.0 || y != 0.0 || z != 0.0)
		{
			node->m[ 0] = 1-2*x*x;
			node->m[ 1] = -2*y*x;
			node->m[ 2] = -2*z*x;

			node->m[ 4] = -2*x*y;
			node->m[ 5] = 1-2*y*y;
			node->m[ 6] = -2*z*y;

			node->m[ 8] = -2*x*z;
			node->m[ 9] = -2*y*z;
			node->m[10] = 1-2*z*z;
		}
	}
	if (type == TRANSLATE)
	{
		Value v = c.lookup_variable("v");
		v.getv3(node->m[12], node->m[13], node->m[14]);
	}
	if (type == MULTMATRIX)
	{
		Value v = c.lookup_variable("m");
		if (v.type == Value::VECTOR) {
			for (int i = 0; i < 16; i++) {
				int x = i / 4, y = i % 4;
				if (y < v.vec.size() && v.vec[y]->type == Value::VECTOR && x < v.vec[y]->vec.size())
					v.vec[y]->vec[x]->getnum(node->m[i]);
			}
		}
	}
	if (type == COLOR)
	{
		Value v = c.lookup_variable("c");
		if (v.type == Value::VECTOR) {
			for (int i = 0; i < 4; i++)
				node->m[16+i] = i < v.vec.size() ? v.vec[i]->num : 1.0;
		}
	}

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n != NULL)
			node->children.append(n);
	}

	return node;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron TransformNode::render_cgal_nef_polyhedron() const
{
	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id)) {
		progress_report();
		PRINT(cgal_nef_cache[cache_id]->msg);
		return cgal_nef_cache[cache_id]->N;
	}

	print_messages_push();

	bool first = true;
	CGAL_Nef_polyhedron N;

	foreach (AbstractNode *v, children) {
		if (v->modinst->tag_background)
			continue;
		if (first) {
			N = v->render_cgal_nef_polyhedron();
			if (N.dim != 0)
				first = false;
		} else if (N.dim == 2) {
			N.p2 += v->render_cgal_nef_polyhedron().p2;
		} else if (N.dim == 3) {
			N.p3 += v->render_cgal_nef_polyhedron().p3;
		}
	}

	if (N.dim == 2)
	{
		// Unfortunately CGAL provides no transform method for CGAL_Nef_polyhedron2
		// objects. So we convert in to our internal 2d data format, transform it,
		// tesselate it and create a new CGAL_Nef_polyhedron2 from it.. What a hack!
		
		CGAL_Aff_transformation2 t(
				m[0], m[4], m[12],
				m[1], m[5], m[13], m[15]);

		DxfData dd(N);
		for (int i=0; i < dd.points.size(); i++) {
			CGAL_Kernel2::Point_2 p = CGAL_Kernel2::Point_2(dd.points[i].x, dd.points[i].y);
			p = t.transform(p);
			dd.points[i].x = to_double(p.x());
			dd.points[i].y = to_double(p.y());
		}

		PolySet ps;
		ps.is2d = true;
		dxf_tesselate(&ps, &dd, 0, true, false, 0);

		N = ps.render_cgal_nef_polyhedron();
		ps.refcount = 0;
	}
	if (N.dim == 3) {
		CGAL_Aff_transformation t(
				m[0], m[4], m[ 8], m[12],
				m[1], m[5], m[ 9], m[13],
				m[2], m[6], m[10], m[14], m[15]);
		N.p3.transform(t);
	}

	cgal_nef_cache.insert(cache_id, new cgal_nef_cache_entry(N), N.weight());
	print_messages_pop();
	progress_report();

	return N;
}

#endif /* ENABLE_CGAL */

CSGTerm *TransformNode::render_csg_term(double c[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	double x[20];

	for (int i = 0; i < 16; i++)
	{
		int c_row = i%4;
		int m_col = i/4;
		x[i] = 0;
		for (int j = 0; j < 4; j++)
			x[i] += c[c_row + j*4] * m[m_col*4 + j];
	}

	for (int i = 16; i < 20; i++)
		x[i] = m[i] < 0 ? c[i] : m[i];

	CSGTerm *t1 = NULL;
	foreach(AbstractNode *v, children)
	{
		CSGTerm *t2 = v->render_csg_term(x, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
		}
	}
	if (t1 && modinst->tag_highlight && highlights)
		highlights->append(t1->link());
	if (t1 && modinst->tag_background && background) {
		background->append(t1);
		return NULL;
	}
	return t1;
}

QString TransformNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		if (m[16] >= 0 || m[17] >= 0 || m[18] >= 0 || m[19] >= 0)
			text.sprintf("n%d: color([%f, %f, %f, %f])", idx,
					m[16], m[17], m[18], m[19]);
		else
			text.sprintf("n%d: multmatrix([[%f, %f, %f, %f], [%f, %f, %f, %f], "
					"[%f, %f, %f, %f], [%f, %f, %f, %f]])", idx,
					m[0], m[4], m[ 8], m[12],
					m[1], m[5], m[ 9], m[13],
					m[2], m[6], m[10], m[14],
					m[3], m[7], m[11], m[15]);
		text = indent + text + " {\n";
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		((AbstractNode*)this)->dump_cache = text + indent + "}\n";
	}
	return dump_cache;
}

void register_builtin_transform()
{
	builtin_modules["scale"] = new TransformModule(SCALE);
	builtin_modules["rotate"] = new TransformModule(ROTATE);
	builtin_modules["mirror"] = new TransformModule(MIRROR);
	builtin_modules["translate"] = new TransformModule(TRANSLATE);
	builtin_modules["multmatrix"] = new TransformModule(MULTMATRIX);
	builtin_modules["color"] = new TransformModule(COLOR);
}

