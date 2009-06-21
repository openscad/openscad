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

class CubeModule : public AbstractModule
{
public:
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class CubeNode : public AbstractNode
{
public:
	double x, y, z;
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
	virtual QString dump(QString indent) const;
};

AbstractNode *CubeModule::evaluate(const Context*, const QVector<QString>&, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const
{
	CubeNode *node = new CubeNode();
	if (call_argvalues.size() == 1 && call_argvalues[0].is_vector) {
		node->x = call_argvalues[0].x;
		node->y = call_argvalues[0].y;
		node->z = call_argvalues[0].z;
	} else if (call_argvalues.size() == 1 && !call_argvalues[0].is_nan) {
		node->x = call_argvalues[0].x;
		node->y = call_argvalues[0].x;
		node->z = call_argvalues[0].x;
	}
	foreach (AbstractNode *v, child_nodes)
		delete v;
	return node;
}

void register_builtin_cube()
{
	builtin_modules["cube"] = new CubeModule();
}

CGAL_Nef_polyhedron CubeNode::render_cgal_nef_polyhedron() const
{
	CGAL_Plane P1 = CGAL_Plane(+1,  0,  0, -x/2);
	CGAL_Nef_polyhedron N1(P1);
	CGAL_Nef_polyhedron N2(CGAL_Plane(-1,  0,  0, -x/2));
	CGAL_Nef_polyhedron N3(CGAL_Plane( 0, +1,  0, -y/2));
	CGAL_Nef_polyhedron N4(CGAL_Plane( 0, -1,  0, -y/2));
	CGAL_Nef_polyhedron N5(CGAL_Plane( 0,  0, +1, -z/2));
	CGAL_Nef_polyhedron N6(CGAL_Plane( 0,  0, -1, -z/2));
	CGAL_Nef_polyhedron N = N1 * N2 * N3 * N4 * N5 * N6;
	progress_report();
	return N;
}

QString CubeNode::dump(QString indent) const
{
	QString text;
	if (x == y && y == z)
		text.sprintf("cube(%f);\n", x);
	else
		text.sprintf("cube([%f %f %f]);\n", x, y, z);
	return indent + text;
}

