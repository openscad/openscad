/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                  Marius Kintel <marius@kintel.net>
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
#include "polyset.h"
#include "context.h"
#include "builtin.h"
#include "dxftess.h"
#include "printutils.h"
#include "openscad.h" // handle_dep()

#include "viewport.h"
#include "GLView.h"

GLView *Viewport::screen = NULL;
bool Viewport::has_run_once = false;

enum viewport_type_e {
	STARTING_VP,
	ANIMATION_VP
};

class ViewportModule : public AbstractModule
{
public:
	viewport_type_e type;
	ViewportModule(viewport_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class ViewportNode : public AbstractPolyNode
{
public:
	QMap<QString, double> parameters;
	viewport_type_e type;
	ViewportNode(const ModuleInstantiation *mi, viewport_type_e type) : AbstractPolyNode(mi), type(type) { }
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *ViewportModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	ViewportNode *node = new ViewportNode(inst, type);

	// unlike other modules, default values are unnecessary. 

	QVector<QString> argnames = QVector<QString>() << "distance";
	argnames << "transx" << "transy" << "transz" ;
	argnames << "rotx" << "roty" << "rotz" ;
	argnames << "translation" << "rotation";

	// "translation" and "rotation" are intended for keyword usage.
	// if the 2nd + 3rd arguments are vectors, they are treated as
	// translation and rotation vectors.

	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value v = c.lookup_variable("distance");
	if (v.type == Value::NUMBER) node->parameters["distance"] = v.num;

	v = c.lookup_variable("transx");
	if (v.type == Value::NUMBER) {
		node->parameters["object_trans_x"] = v.num;
	} else if (v.type == Value::VECTOR) {
		v.getv3(node->parameters["object_trans_x"],
			node->parameters["object_trans_y"],
			node->parameters["object_trans_z"]);
	}

	v = c.lookup_variable("transy");
	if (v.type == Value::NUMBER) {
		node->parameters["object_trans_y"] = v.num;
	} else if (v.type == Value::VECTOR) {
		v.getv3(node->parameters["object_rot_x"],
			node->parameters["object_rot_y"],
			node->parameters["object_rot_z"]);
	}
	v = c.lookup_variable("transz");
	if (v.type == Value::NUMBER) node->parameters["object_trans_z"] = v.num;

	v = c.lookup_variable("rotx");
	if (v.type == Value::NUMBER) node->parameters["object_rot_x"] = v.num;
	v = c.lookup_variable("roty");
	if (v.type == Value::NUMBER) node->parameters["object_rot_y"] = v.num;
	v = c.lookup_variable("rotz");
	if (v.type == Value::NUMBER) node->parameters["object_rot_z"] = v.num;

	v = c.lookup_variable("translation");
	if (v.type == Value::VECTOR) {
		v.getv3(node->parameters["object_trans_x"],
			node->parameters["object_trans_y"],
			node->parameters["object_trans_z"]);
	}

	v = c.lookup_variable("rotation");
	if (v.type == Value::VECTOR) {
		v.getv3(node->parameters["object_rot_x"],
			node->parameters["object_rot_y"],
			node->parameters["object_rot_z"]);
	}

	return node;
}

void register_builtin_viewport()
{
	builtin_modules["starting_viewport"] = new ViewportModule(STARTING_VP);
	builtin_modules["animation_viewport"] = new ViewportModule(ANIMATION_VP);
}

PolySet *ViewportNode::render_polyset(render_mode_e) const
{
	// this does not render anything. it changes various aspects of the viewport.
	if (Viewport::screen) {
		if ( type == ANIMATION_VP || (type == STARTING_VP && !Viewport::has_run_once) ) {
			if (parameters.contains("distance"))
				Viewport::screen->viewer_distance = parameters.value("distance");
			if (parameters.contains("object_trans_x"))
				Viewport::screen->object_trans_x = -1 * parameters.value("object_trans_x");
			if (parameters.contains("object_trans_y"))
				Viewport::screen->object_trans_y = -1 * parameters.value("object_trans_y");
			if (parameters.contains("object_trans_z"))
				Viewport::screen->object_trans_z = -1 * parameters.value("object_trans_z");
			if (parameters.contains("object_rot_x"))
				Viewport::screen->object_rot_x = 360-parameters.value("object_rot_x")+90;
			if (parameters.contains("object_rot_y"))
				Viewport::screen->object_rot_y = 360-parameters.value("object_rot_y");
			if (parameters.contains("object_rot_z"))
				Viewport::screen->object_rot_z = 360-parameters.value("object_rot_z");
			if ( type == STARTING_VP ) Viewport::screen->updateGL();
			Viewport::has_run_once = true;
		}
	}
	PolySet *p = new PolySet();
	return p;
}

QString ViewportNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString header,text;
		if (type == STARTING_VP) header += "starting_viewport(";
		if (type == ANIMATION_VP) header += "animation_viewport(";
		text.sprintf("distance = %g, translation = [%g, %g, %g], rotation = [%g, %g, %g] );\n", 
			parameters["distance"], 
			parameters["object_trans_x"],
			parameters["object_trans_y"],
			parameters["object_trans_z"],
			parameters["object_rot_x"],
			parameters["object_rot_y"],
			parameters["object_rot_z"]);
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + header + text;
	}
	return dump_cache;
}

