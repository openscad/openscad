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

enum csg_type_e {
	UNION,
	DIFFERENCE,
	INTERSECTION
};

class CsgModule : public AbstractModule
{
public:
	csg_type_e type;
	CsgModule(csg_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class CsgNode : public AbstractNode
{
public:
	csg_type_e type;
	CsgNode(csg_type_e type) : type(type) { }
#ifdef ENABLE_CGAL
        virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	CSGTerm *render_csg_term(double m[16]) const;
        virtual QString dump(QString indent) const;
};

AbstractNode *CsgModule::evaluate(const Context*, const QVector<QString>&, const QVector<Value>&, const QVector<AbstractNode*> child_nodes) const
{
	if (child_nodes.size() == 1)
		return child_nodes[0];

	CsgNode *node = new CsgNode(type);
	foreach (AbstractNode *v, child_nodes)
		node->children.append(v);
	return node;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron CsgNode::render_cgal_nef_polyhedron() const
{
	bool first;
	CGAL_Nef_polyhedron N;
	foreach (AbstractNode *v, children) {
		if (first) {
			N = v->render_cgal_nef_polyhedron();
			first = false;
		} else if (type == UNION) {
			N += v->render_cgal_nef_polyhedron();
		} else if (type == DIFFERENCE) {
			N -= v->render_cgal_nef_polyhedron();
		} else if (type == INTERSECTION) {
			N *= v->render_cgal_nef_polyhedron();
		}
	}
	progress_report();
	return N;
}

#endif /* ENABLE_CGAL */

CSGTerm *CsgNode::render_csg_term(double m[16]) const
{
	CSGTerm *t1 = NULL;
	foreach (AbstractNode *v, children) {
		CSGTerm *t2 = v->render_csg_term(m);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (type == UNION) {
				t1 = new CSGTerm(CSGTerm::UNION, t1, t2);
			} else if (type == DIFFERENCE) {
				t1 = new CSGTerm(CSGTerm::DIFFERENCE, t1, t2);
			} else if (type == INTERSECTION) {
				t1 = new CSGTerm(CSGTerm::INTERSECTION, t1, t2);
			}
		}
	}
	return t1;
}

QString CsgNode::dump(QString indent) const
{
	QString text = indent + QString("n%1: ").arg(idx);
	if (type == UNION)
		text += "union() {\n";
	if (type == DIFFERENCE)
		text += "difference() {\n";
	if (type == INTERSECTION)
		text += "intersection() {\n";
	foreach (AbstractNode *v, children)
		text += v->dump(indent + QString("\t"));
	return text + indent + "}\n";
}

void register_builtin_csg()
{
	builtin_modules["union"] = new CsgModule(UNION);
	builtin_modules["difference"] = new CsgModule(DIFFERENCE);
	builtin_modules["intersection"] = new CsgModule(INTERSECTION);
}

