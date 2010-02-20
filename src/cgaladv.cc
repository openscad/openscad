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
#include "builtin.h"
#include "printutils.h"
#include "cgal.h"

#include <CGAL/minkowski_sum_3.h>

enum cgaladv_type_e {
	MINKOWSKI,
	GLIDE,
	SUBDIV
};

class CgaladvModule : public AbstractModule
{
public:
	cgaladv_type_e type;
	CgaladvModule(cgaladv_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class CgaladvNode : public AbstractNode
{
public:
	Value path;
	QString subdiv_type;
	int convexity, level;
	cgaladv_type_e type;
	CgaladvNode(const ModuleInstantiation *mi, cgaladv_type_e type) : AbstractNode(mi), type(type) {
		convexity = 1;
	}
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *CgaladvModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	CgaladvNode *node = new CgaladvNode(inst, type);

	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	if (type == MINKOWSKI)
		argnames = QVector<QString>() << "convexity";

	if (type == GLIDE)
		argnames = QVector<QString>() << "path" << "convexity";

	if (type == SUBDIV)
		argnames = QVector<QString>() << "type" << "level" << "convexity";

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value convexity, path, subdiv_type, level;

	if (type == MINKOWSKI) {
		convexity = c.lookup_variable("convexity", true);
	}

	if (type == GLIDE) {
		convexity = c.lookup_variable("convexity", true);
		path = c.lookup_variable("path", false);
	}

	if (type == SUBDIV) {
		convexity = c.lookup_variable("convexity", true);
		subdiv_type = c.lookup_variable("type", false);
		level = c.lookup_variable("level", true);
	}

	node->convexity = (int)convexity.num;
	node->path = path;
	node->subdiv_type = subdiv_type.text;
	node->level = (int)level.num;

	if (node->level <= 1)
		node->level = 1;

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n)
			node->children.append(n);
	}

	return node;
}

void register_builtin_cgaladv()
{
	builtin_modules["minkowski"] = new CgaladvModule(MINKOWSKI);
	builtin_modules["glide"] = new CgaladvModule(GLIDE);
	builtin_modules["subdiv"] = new CgaladvModule(SUBDIV);
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron CgaladvNode::render_cgal_nef_polyhedron() const
{
	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id)) {
		progress_report();
		PRINT(cgal_nef_cache[cache_id]->msg);
		return cgal_nef_cache[cache_id]->N;
	}

	print_messages_push();
	CGAL_Nef_polyhedron N;

	if (type == MINKOWSKI)
	{
		bool first = true;
		CGAL_Nef_polyhedron a, b;
		foreach(AbstractNode * v, children) {
			if (v->modinst->tag_background)
				continue;
			if (first) {
				a = v->render_cgal_nef_polyhedron();
				if (a.dim != 0)
					first = false;
			} else {
				b += v->render_cgal_nef_polyhedron();
			}
		}
		if (a.dim == 3 && b.dim == 3) {
			N.dim = 3;
			N.p3 = CGAL::minkowski_sum_3(a.p3, b.p3);
		}
		if (a.dim == 2 && b.dim == 2) {
			PRINT("WARNING: minkowski() is not implemented yet for 2d objects!");
		}
	}

	if (type == GLIDE)
	{
		PRINT("WARNING: subdiv() is not implemented yet!");
	}

	if (type == SUBDIV)
	{
		PRINT("WARNING: subdiv() is not implemented yet!");
	}

	cgal_nef_cache.insert(cache_id, new cgal_nef_cache_entry(N), N.weight());
	print_messages_pop();
	progress_report();

	return N;
}

CSGTerm *CgaladvNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	if (type == MINKOWSKI)
		return render_csg_term_from_nef(m, highlights, background, "minkowski", this->convexity);

	if (type == GLIDE)
		return render_csg_term_from_nef(m, highlights, background, "glide", this->convexity);

	if (type == SUBDIV)
		return render_csg_term_from_nef(m, highlights, background, "subdiv", this->convexity);

	return NULL;
}

#else // ENABLE_CGAL

CSGTerm *CgaladvNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	PRINT("WARNING: Found minkowski(), glide() or subdiv() statement but compiled without CGAL support!");
	return NULL;
}

#endif // ENABLE_CGAL

QString CgaladvNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		if (type == MINKOWSKI)
			text.sprintf("minkowski(convexity = %d) {\n", this->convexity);
		if (type == GLIDE) {
			text.sprintf(", convexity = %d) {\n", this->convexity);
			text = QString("glide(path = ") + this->path.dump() + text;
		}
		if (type == SUBDIV)
			text.sprintf("subdiv(level = %d, convexity = %d) {\n", this->level, this->convexity);
		foreach (AbstractNode *v, this->children)
			text += v->dump(indent + QString("\t"));
		text += indent + "}\n";
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}

