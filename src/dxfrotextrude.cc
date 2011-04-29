/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
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

#include "dxfrotextrudenode.h"
#include "module.h"
#include "context.h"
#include "printutils.h"
#include "builtin.h"
#include "polyset.h"
#include "dxfdata.h"
#include "progress.h"
#include "visitor.h"
#include "PolySetRenderer.h"
#include "openscad.h" // get_fragments_from_r()

#include <sstream>

#include <QTime>
#include <QApplication>
#include <QProgressDialog>
#include <QDateTime>
#include <QFileInfo>

class DxfRotateExtrudeModule : public AbstractModule
{
public:
	DxfRotateExtrudeModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
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

	if (!file.text.empty())
		node->filename = c.get_absolute_path(QString::fromStdString(file.text));

	node->layername = QString::fromStdString(layer.text);
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

PolySet *DxfRotateExtrudeNode::render_polyset(render_mode_e mode, 
																							PolySetRenderer *renderer) const
{
	if (!renderer) {
		PRINTF("WARNING: No suitable PolySetRenderer found for %s module!", this->name().c_str());
		PolySet *ps = new PolySet();
		ps->is2d = true;
		return ps;
	}

	print_messages_push();

	PolySet *ps = renderer->renderPolySet(*this, mode);
	
	print_messages_pop();

	return ps;
}

std::string DxfRotateExtrudeNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "("
		"file = \"" << this->filename << "\", "
		"cache = \"" << QFileInfo(this->filename) << "\", "
		"layer = \"" << this->layername << "\", "
		"origin = [ " << std::dec << this->origin_x << " " << this->origin_y << " ], "
		"scale = " << this->scale << ", "
		"convexity = " << this->convexity << ", "
		"$fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs << ")";

	return stream.str();
}
