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
#include "printutils.h"

AbstractModule::~AbstractModule()
{
}

AbstractNode *AbstractModule::evaluate(const Context*, const ModuleInstantiation *inst) const
{
	AbstractNode *node = new AbstractNode(inst);

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n)
			node->children.append(n);
	}

	return node;
}

QString AbstractModule::dump(QString indent, QString name) const
{
	return QString("%1abstract module %2();\n").arg(indent, name);
}

ModuleInstantiation::~ModuleInstantiation()
{
	foreach (Expression *v, argexpr)
		delete v;
	foreach (ModuleInstantiation *v, children)
		delete v;
}

QString ModuleInstantiation::dump(QString indent) const
{
	QString text = indent;
	if (!label.isEmpty())
		text += label + QString(": ");
	text += modname + QString("(");
	for (int i=0; i < argnames.size(); i++) {
		if (i > 0)
			text += QString(", ");
		if (!argnames[i].isEmpty())
			text += argnames[i] + QString(" = ");
		text += argexpr[i]->dump();
	}
	if (children.size() == 0) {
		text += QString(");\n");
	} else if (children.size() == 1) {
		text += QString(")\n");
		text += children[0]->dump(indent + QString("\t"));
	} else {
		text += QString(") {\n");
		for (int i = 0; i < children.size(); i++) {
			text += children[i]->dump(indent + QString("\t"));
		}
		text += QString("%1}\n").arg(indent);
	}
	return text;
}

AbstractNode *ModuleInstantiation::evaluate(const Context *ctx) const
{
	AbstractNode *node = NULL;
	if (this->ctx) {
		PRINTA("WARNING: Ignoring recursive module instanciation of '%1'.", modname);
	} else {
		ModuleInstantiation *that = (ModuleInstantiation*)this;
		that->argvalues.clear();
		foreach (Expression *v, that->argexpr) {
			that->argvalues.append(v->evaluate(ctx));
		}
		that->ctx = ctx;
		node = ctx->evaluate_module(this);
		that->ctx = NULL;
		that->argvalues.clear();
	}
	return node;
}

Module::~Module()
{
	foreach (Expression *v, assignments_expr)
		delete v;
	foreach (AbstractFunction *v, functions)
		delete v;
	foreach (AbstractModule *v, modules)
		delete v;
	foreach (ModuleInstantiation *v, children)
		delete v;
}

AbstractNode *Module::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	c.functions_p = &functions;
	c.modules_p = &modules;

	for (int i = 0; i < assignments_var.size(); i++) {
		c.set_variable(assignments_var[i], assignments_expr[i]->evaluate(&c));
	}

	AbstractNode *node = new AbstractNode(inst);
	for (int i = 0; i < children.size(); i++) {
		AbstractNode *n = children[i]->evaluate(&c);
		if (n != NULL)
			node->children.append(n);
	}

	foreach(ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n != NULL)
			node->children.append(n);
	}

	return node;
}

QString Module::dump(QString indent, QString name) const
{
	QString text, tab;
	if (!name.isEmpty()) {
		text = QString("%1module %2(").arg(indent, name);
		for (int i=0; i < argnames.size(); i++) {
			if (i > 0)
				text += QString(", ");
			text += argnames[i];
			if (argexpr[i])
				text += QString(" = ") + argexpr[i]->dump();
		}
		text += QString(") {\n");
		tab = "\t";
	}
	{
		QHashIterator<QString, AbstractFunction*> i(functions);
		while (i.hasNext()) {
			i.next();
			text += i.value()->dump(indent + tab, i.key());
		}
	}
	{
		QHashIterator<QString, AbstractModule*> i(modules);
		while (i.hasNext()) {
			i.next();
			text += i.value()->dump(indent + tab, i.key());
		}
	}
	for (int i = 0; i < assignments_var.size(); i++) {
		text += QString("%1%2 = %3;\n").arg(indent + tab, assignments_var[i], assignments_expr[i]->dump());
	}
	for (int i = 0; i < children.size(); i++) {
		text += children[i]->dump(indent + tab);
	}
	if (!name.isEmpty()) {
		text += QString("%1}\n").arg(indent);
	}
	return text;
}

QHash<QString, AbstractModule*> builtin_modules;

void initialize_builtin_modules()
{
	builtin_modules["group"] = new AbstractModule();

	register_builtin_csgops();
	register_builtin_transform();
	register_builtin_primitives();
	register_builtin_surface();
	register_builtin_control();
	register_builtin_render();
	register_builtin_import();
	register_builtin_dxf_linear_extrude();
	register_builtin_dxf_rotate_extrude();
}

void destroy_builtin_modules()
{
	foreach (AbstractModule *v, builtin_modules)
		delete v;
	builtin_modules.clear();
}

int AbstractNode::idx_counter;

AbstractNode::AbstractNode(const ModuleInstantiation *mi)
{
	modinst = mi;
	idx = idx_counter++;
}

AbstractNode::~AbstractNode()
{
	foreach (AbstractNode *v, children)
		delete v;
}

QString AbstractNode::mk_cache_id() const
{
	QString cache_id = dump("");
	cache_id.remove(QRegExp("[a-zA-Z_][a-zA-Z_0-9]*:"));
	cache_id.remove(' ');
	cache_id.remove('\t');
	cache_id.remove('\n');
	return cache_id;
}

#ifdef ENABLE_CGAL

AbstractNode::cgal_nef_cache_entry::cgal_nef_cache_entry(CGAL_Nef_polyhedron N) :
		N(N), msg(print_messages_stack.last()) { };

QCache<QString, AbstractNode::cgal_nef_cache_entry> AbstractNode::cgal_nef_cache(100000);

static CGAL_Nef_polyhedron render_cgal_nef_polyhedron_backend(const AbstractNode *that, bool intersect)
{
	QString cache_id = that->mk_cache_id();
	if (that->cgal_nef_cache.contains(cache_id)) {
		that->progress_report();
		PRINT(that->cgal_nef_cache[cache_id]->msg);
		return that->cgal_nef_cache[cache_id]->N;
	}

	print_messages_push();

	bool first = true;
	CGAL_Nef_polyhedron N;
	foreach (AbstractNode *v, that->children) {
		if (v->modinst->tag_background)
			continue;
		if (first) {
			N = v->render_cgal_nef_polyhedron();
			if (N.dim != 0)
				first = false;
		} else if (N.dim == 2) {
			if (intersect)
				N.p2 *= v->render_cgal_nef_polyhedron().p2;
			else
				N.p2 += v->render_cgal_nef_polyhedron().p2;
		} else {
			if (intersect)
				N.p3 *= v->render_cgal_nef_polyhedron().p3;
			else
				N.p3 += v->render_cgal_nef_polyhedron().p3;
		}
	}

	that->cgal_nef_cache.insert(cache_id, new AbstractNode::cgal_nef_cache_entry(N), N.weight());
	that->progress_report();
	print_messages_pop();

	return N;
}

CGAL_Nef_polyhedron AbstractNode::render_cgal_nef_polyhedron() const
{
	return render_cgal_nef_polyhedron_backend(this, false);
}

CGAL_Nef_polyhedron AbstractIntersectionNode::render_cgal_nef_polyhedron() const
{
	return render_cgal_nef_polyhedron_backend(this, true);
}

#endif /* ENABLE_CGAL */

static CSGTerm *render_csg_term_backend(const AbstractNode *that, bool intersect, double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background)
{
	CSGTerm *t1 = NULL;
	foreach(AbstractNode *v, that->children) {
		CSGTerm *t2 = v->render_csg_term(m, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (intersect)
				t1 = new CSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			else
				t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
		}
	}
	if (t1 && that->modinst->tag_highlight && highlights)
		highlights->append(t1->link());
	if (t1 && that->modinst->tag_background && background) {
		background->append(t1);
		return NULL;
	}
	return t1;
}

CSGTerm *AbstractNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	return render_csg_term_backend(this, false, m, highlights, background);
}

CSGTerm *AbstractIntersectionNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	return render_csg_term_backend(this, true, m, highlights, background);
}

QString AbstractNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text = indent + QString("n%1: group() {\n").arg(idx);
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		((AbstractNode*)this)->dump_cache = text + indent + "}\n";
	}
	return dump_cache;
}

QString AbstractIntersectionNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text = indent + QString("n%1: intersection() {\n").arg(idx);
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		((AbstractNode*)this)->dump_cache = text + indent + "}\n";
	}
	return dump_cache;
}

int progress_report_count;
void (*progress_report_f)(const class AbstractNode*, void*, int);
void *progress_report_vp;

void AbstractNode::progress_prepare()
{
	foreach (AbstractNode *v, children)
		v->progress_prepare();
	progress_mark = ++progress_report_count;
}

void AbstractNode::progress_report() const
{
	if (progress_report_f)
		progress_report_f(this, progress_report_vp, progress_mark);
}

void progress_report_prep(AbstractNode *root, void (*f)(const class AbstractNode *node, void *vp, int mark), void *vp)
{
	progress_report_count = 0;
	progress_report_f = f;
	progress_report_vp = vp;
	root->progress_prepare();
}

void progress_report_fin()
{
	progress_report_count = 0;
	progress_report_f = NULL;
	progress_report_vp = NULL;
}

