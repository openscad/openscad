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

class TransModule : public AbstractModule
{
public:
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class TransNode : public AbstractNode
{
public:
	double x, y, z;
#ifdef ENABLE_CGAL
        virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
        virtual QString dump(QString indent) const;
};

AbstractNode *TransModule::evaluate(const Context*, const QVector<QString>&, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const
{
	TransNode *node = new TransNode();
	if (call_argvalues.size() == 1 && call_argvalues[0].type == Value::VECTOR) {
		node->x = call_argvalues[0].x;
		node->y = call_argvalues[0].y;
		node->z = call_argvalues[0].z;
	} else {
		node->x = 0;
		node->y = 0;
		node->z = 0;
	}
	foreach (AbstractNode *v, child_nodes)
		node->children.append(v);
	return node;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron TransNode::render_cgal_nef_polyhedron() const
{
	CGAL_Nef_polyhedron N;
	foreach (AbstractNode *v, children)
		N += v->render_cgal_nef_polyhedron();
	CGAL_Aff_transformation t(CGAL::TRANSLATION, CGAL_Vector(x, y, z));
	N.transform(t);
	progress_report();
	return N;
}

#endif /* ENABLE_CGAL */

QString TransNode::dump(QString indent) const
{
	QString text;
	text.sprintf("trans([%f %f %f])", x, y, z);
	text = indent + text + " {\n";
	foreach (AbstractNode *v, children)
		text += v->dump(indent + QString("\t"));
	return text + indent + "}\n";
}

void register_builtin_trans()
{
	builtin_modules["trans"] = new TransModule();
}

