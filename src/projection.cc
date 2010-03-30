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

#include "projectionnode.h"
#include "module.h"
#include "context.h"
#include "printutils.h"
#include "builtin.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "polyset.h"
#include "export.h"
#include "progress.h"
#include "visitor.h"
#include "PolySetRenderer.h"

#ifdef ENABLE_CGAL
#  include <CGAL/assertions_behaviour.h>
#  include <CGAL/exceptions.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <sstream>

#include <QApplication>
#include <QTime>
#include <QProgressDialog>

class ProjectionModule : public AbstractModule
{
public:
	ProjectionModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *ProjectionModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	ProjectionNode *node = new ProjectionNode(inst);

	QVector<QString> argnames = QVector<QString>() << "cut";
	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value convexity = c.lookup_variable("convexity", true);
	Value cut = c.lookup_variable("cut", true);

	node->convexity = (int)convexity.num;

	if (cut.type == Value::BOOL)
		node->cut_mode = cut.b;

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n)
			node->children.append(n);
	}

	return node;
}

void register_builtin_projection()
{
	builtin_modules["projection"] = new ProjectionModule();
}

PolySet *ProjectionNode::render_polyset(render_mode_e mode) const
{
	PolySetRenderer *renderer = PolySetRenderer::renderer();
	if (!renderer) {
		PRINT("WARNING: No suitable PolySetRenderer found for projection() module!");
		PolySet *ps = new PolySet();
		ps->is2d = true;
		return ps;
	}

	QString key = mk_cache_id();
	if (PolySet::ps_cache.contains(key)) {
		PRINT(PolySet::ps_cache[key]->msg);
		return PolySet::ps_cache[key]->ps->link();
	}

	print_messages_push();

	PolySet *ps = renderer->renderPolySet(*this, mode);
	PolySet::ps_cache.insert(key, new PolySet::ps_cache_entry(ps->link()));

	print_messages_pop();

	return ps;
}

std::string ProjectionNode::toString() const
{
	std::stringstream stream;
	stream << "n" << this->index() << ": ";

	stream << "projection(cut = " << (this->cut_mode ? "true" : "false")
				 << ", convexity = " << this->convexity << ")";

	return stream.str();
}
