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

class UnionModule : public AbstractModule
{
public:
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class UnionNode : public AbstractNode
{
public:
        virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
        virtual QString dump(QString indent) const;
};

AbstractNode *UnionModule::evaluate(const Context*, const QVector<QString>&, const QVector<Value>&, const QVector<AbstractNode*> child_nodes) const
{
	UnionNode *node = new UnionNode();
	foreach (AbstractNode *v, child_nodes)
		node->children.append(v);
	return node;
}

CGAL_Nef_polyhedron UnionNode::render_cgal_nef_polyhedron() const
{
	CGAL_Nef_polyhedron N;
	foreach (AbstractNode *v, children)
		N += v->render_cgal_nef_polyhedron();
	progress_report();
	return N;
}

QString UnionNode::dump(QString indent) const
{
	QString text = indent + "union() {\n";
	foreach (AbstractNode *v, children)
		text += v->dump(indent + QString("\t"));
	return text + indent + "}\n";
}

void register_builtin_union()
{
	builtin_modules["union"] = new UnionModule();
}

