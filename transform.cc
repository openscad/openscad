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

enum transform_type_e {
	SCALE,
	ROTATE,
	TRANSLATE,
	MULTMATRIX
};

class TransformModule : public AbstractModule
{
public:
	transform_type_e type;
	TransformModule(transform_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class TransformNode : public AbstractNode
{
public:
	double m[16];
#ifdef ENABLE_CGAL
        virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[16]) const;
        virtual QString dump(QString indent) const;
};

AbstractNode *TransformModule::evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const
{
	TransformNode *node = new TransformNode();

	for (int i = 0; i < 16; i++) {
		node->m[i] = i % 5 == 0 ? 1.0 : 0.0;
	}

	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	if (type == SCALE) {
		argnames = QVector<QString>() << "v";
	}
	if (type == ROTATE) {
		argnames = QVector<QString>() << "a" << "v";
	}
	if (type == TRANSLATE) {
		argnames = QVector<QString>() << "v";
	}
	if (type == MULTMATRIX) {
		argnames = QVector<QString>() << "m";
	}

        Context c(ctx);
        c.args(argnames, argexpr, call_argnames, call_argvalues);

	if (type == SCALE)
	{
		Value v = c.lookup_variable("v");
		if (v.type == Value::NUMBER) {
			node->m[0] = v.num;
			node->m[5] = v.num;
			node->m[10] = v.num;
		}
		if (v.type == Value::VECTOR) {
			node->m[0] = v.x;
			node->m[5] = v.y;
			node->m[10] = v.z;
		}
	}
	if (type == ROTATE)
	{
		Value val_a = c.lookup_variable("a");
		Value val_v = c.lookup_variable("v");
		double a = 0, x = 0, y = 0, z = 1;

		if (val_a.type == Value::NUMBER) {
			a = val_a.num;
		}

		if (val_v.type == Value::VECTOR) {
			x = val_v.x; y = val_v.y; z = val_v.z;
			double sn = 1.0 / sqrt(x*x + y*y + z*z);
			x *= sn; y *= sn; z *= sn;
		}

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
	if (type == TRANSLATE)
	{
		Value v = c.lookup_variable("v");
		if (v.type == Value::VECTOR) {
			node->m[12] = v.x;
			node->m[13] = v.y;
			node->m[14] = v.z;
		}
	}
	if (type == MULTMATRIX)
	{
		Value v = c.lookup_variable("m");
		if (v.type == Value::MATRIX) {
			for (int i = 0; i < 16; i++)
				node->m[i] = v.m[i];
		}
	}

	foreach (AbstractNode *v, child_nodes)
		node->children.append(v);

	return node;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron TransformNode::render_cgal_nef_polyhedron() const
{
	CGAL_Nef_polyhedron N;
	foreach (AbstractNode *v, children)
		N += v->render_cgal_nef_polyhedron();
	CGAL_Aff_transformation t(
			m[0], m[4], m[ 8], m[12],
			m[1], m[5], m[ 9], m[13],
			m[2], m[6], m[10], m[14], m[15]);
	N.transform(t);
	progress_report();
	return N;
}

#endif /* ENABLE_CGAL */

CSGTerm *TransformNode::render_csg_term(double c[16]) const
{
	double x[16];

	for (int i = 0; i < 16; i++)
	{
		int c_row = i%4;
		int m_col = i/4;
		x[i] = 0;
		for (int j = 0; j < 4; j++)
			x[i] += c[c_row + j*4] * m[m_col*4 + j];
	}

	CSGTerm *t1 = NULL;
	foreach(AbstractNode *v, children)
	{
		CSGTerm *t2 = v->render_csg_term(x);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			t1 = new CSGTerm(CSGTerm::UNION, t1, t2);
		}
	}
	return t1;
}

QString TransformNode::dump(QString indent) const
{
	QString text;
	text.sprintf("n%d: multmatrix([%f %f %f %f; %f %f %f %f; %f %f %f %f; %f %f %f %f])", idx,
			m[0], m[4], m[ 8], m[12],
			m[1], m[5], m[ 9], m[13],
			m[2], m[6], m[10], m[14],
			m[3], m[7], m[11], m[15]);
	text = indent + text + " {\n";
	foreach (AbstractNode *v, children)
		text += v->dump(indent + QString("\t"));
	return text + indent + "}\n";
}

void register_builtin_trans()
{
	builtin_modules["scale"] = new TransformModule(SCALE);
	builtin_modules["rotate"] = new TransformModule(ROTATE);
	builtin_modules["translate"] = new TransformModule(TRANSLATE);
	builtin_modules["multmatrix"] = new TransformModule(MULTMATRIX);
}

