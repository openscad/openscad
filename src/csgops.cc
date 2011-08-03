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

#include "module.h"
#include "node.h"
#include "csgterm.h"
#include "builtin.h"
#include "printutils.h"
#ifdef ENABLE_CGAL
#  include "cgal.h"
#  include <CGAL/assertions_behaviour.h>
#  include <CGAL/exceptions.h>
#endif

enum csg_type_e {
	CSG_TYPE_UNION,
	CSG_TYPE_DIFFERENCE,
	CSG_TYPE_INTERSECTION
};

class CsgModule : public AbstractModule
{
public:
	csg_type_e type;
	CsgModule(csg_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class CsgNode : public AbstractNode
{
public:
	csg_type_e type;
	CsgNode(const ModuleInstantiation *mi, csg_type_e type) : AbstractNode(mi), type(type) { }
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *CsgModule::evaluate(const Context*, const ModuleInstantiation *inst) const
{
	CsgNode *node = new CsgNode(inst, type);
	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n != NULL)
			node->children.append(n);
	}
	return node;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron CsgNode::render_cgal_nef_polyhedron() const
{
	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id)) {
		progress_report();
		PRINT(cgal_nef_cache[cache_id]->msg);
		return cgal_nef_cache[cache_id]->N;
	}

	print_messages_push();

	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	bool first = true;
	CGAL_Nef_polyhedron N;
	try {
	foreach (AbstractNode *v, children) {
		if (v->modinst->tag_background)
			continue;
		if (first) {
			N = v->render_cgal_nef_polyhedron();
			if (N.dim != 0)
				first = false;
		} else if (N.dim == 2) {
			if (type == CSG_TYPE_UNION) {
				N.p2 += v->render_cgal_nef_polyhedron().p2;
			} else if (type == CSG_TYPE_DIFFERENCE) {
				N.p2 -= v->render_cgal_nef_polyhedron().p2;
			} else if (type == CSG_TYPE_INTERSECTION) {
				N.p2 *= v->render_cgal_nef_polyhedron().p2;
			}
		} else if (N.dim == 3) {
			if (type == CSG_TYPE_UNION) {
				N.p3 += v->render_cgal_nef_polyhedron().p3;
			} else if (type == CSG_TYPE_DIFFERENCE) {
				N.p3 -= v->render_cgal_nef_polyhedron().p3;
			} else if (type == CSG_TYPE_INTERSECTION) {
				N.p3 *= v->render_cgal_nef_polyhedron().p3;
			}
		}
		v->progress_report();
	}
	cgal_nef_cache.insert(cache_id, new cgal_nef_cache_entry(N), N.weight());
	}
	catch (CGAL::Assertion_exception e) {
		PRINTF("CGAL error: %s", e.what());
	}
	CGAL::set_error_behaviour(old_behaviour);

	print_messages_pop();
	progress_report();

	return N;
}

#endif /* ENABLE_CGAL */

CSGTerm *CsgNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	CSGTerm *t1 = NULL;
	foreach (AbstractNode *v, children) {
		CSGTerm *t2 = v->render_csg_term(m, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (type == CSG_TYPE_UNION) {
				t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
			} else if (type == CSG_TYPE_DIFFERENCE) {
				t1 = new CSGTerm(CSGTerm::TYPE_DIFFERENCE, t1, t2);
			} else if (type == CSG_TYPE_INTERSECTION) {
				t1 = new CSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			}
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

QString CsgNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text = indent + QString("n%1: ").arg(idx);
		if (type == CSG_TYPE_UNION)
			text += "union() {\n";
		if (type == CSG_TYPE_DIFFERENCE)
			text += "difference() {\n";
		if (type == CSG_TYPE_INTERSECTION)
			text += "intersection() {\n";
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		((AbstractNode*)this)->dump_cache = text + indent + "}\n";
	}
	return dump_cache;
}

void register_builtin_csgops()
{
	builtin_modules["union"] = new CsgModule(CSG_TYPE_UNION);
	builtin_modules["difference"] = new CsgModule(CSG_TYPE_DIFFERENCE);
	builtin_modules["intersection"] = new CsgModule(CSG_TYPE_INTERSECTION);
}

