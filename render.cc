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

#include <QProgressDialog>
#include <QApplication>
#include <QTime>

class RenderModule : public AbstractModule
{
public:
	RenderModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstanciation *inst) const;
};

class RenderNode : public AbstractNode
{
public:
	int convexity;
	RenderNode(const ModuleInstanciation *mi) : AbstractNode(mi), convexity(1) { }
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	CSGTerm *render_csg_term(double m[16], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *RenderModule::evaluate(const Context *ctx, const ModuleInstanciation *inst) const
{
	RenderNode *node = new RenderNode(inst);

	QVector<QString> argnames = QVector<QString>() << "convexity";
	QVector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value v = c.lookup_variable("convexity");
	if (v.type == Value::NUMBER)
		node->convexity = (int)v.num;

	foreach (ModuleInstanciation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n != NULL)
			node->children.append(n);
	}

	return node;
}

void register_builtin_render()
{
	builtin_modules["render"] = new RenderModule();
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron RenderNode::render_cgal_nef_polyhedron() const
{
	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id)) {
		progress_report();
		return *cgal_nef_cache[cache_id];
	}

	bool first = true;
	CGAL_Nef_polyhedron N;
	foreach(AbstractNode * v, children)
	{
		if (v->modinst->tag_background)
			continue;
		if (first) {
			N = v->render_cgal_nef_polyhedron();
			if (N.dim != 0)
				first = false;
		} else if (N.dim == 2) {
			N.p2 += v->render_cgal_nef_polyhedron().p2;
		} else if (N.dim == 3) {
			N.p3 += v->render_cgal_nef_polyhedron().p3;
		}
	}

	cgal_nef_cache.insert(cache_id, new CGAL_Nef_polyhedron(N), N.weight());
	progress_report();
	return N;
}

static void report_func(const class AbstractNode*, void *vp, int mark)
{
	QProgressDialog *pd = (QProgressDialog*)vp;
	int v = (int)((mark*100.0) / progress_report_count);
	pd->setValue(v < 100 ? v : 99);
	QString label;
	label.sprintf("Rendering Polygon Mesh using CGAL (%d/%d)", mark, progress_report_count);
	pd->setLabelText(label);
	QApplication::processEvents();
}

CSGTerm *RenderNode::render_csg_term(double m[16], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	QString key = mk_cache_id();
	if (PolySet::ps_cache.contains(key))
		return AbstractPolyNode::render_csg_term_from_ps(m, highlights, background,
				PolySet::ps_cache[key]->ps->link(), modinst, idx);

	CGAL_Nef_polyhedron N;

	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id))
	{
		N = *cgal_nef_cache[cache_id];
	}
	else
	{
		PRINT("Processing uncached render statement...");
		// PRINTA("Cache ID: %1", cache_id);
		QApplication::processEvents();

		QTime t;
		t.start();

		QProgressDialog *pd = new QProgressDialog("Rendering Polygon Mesh using CGAL...", QString(), 0, 100);
		pd->setValue(0);
		pd->setAutoClose(false);
		pd->show();
		QApplication::processEvents();

		progress_report_prep((AbstractNode*)this, report_func, pd);
		N = this->render_cgal_nef_polyhedron();
		progress_report_fin();

		int s = t.elapsed() / 1000;
		PRINTF("..rendering time: %d hours, %d minutes, %d seconds", s / (60*60), (s / 60) % 60, s % 60);

		delete pd;
	}

	PolySet *ps = NULL;

	if (N.dim == 2)
	{
		DxfData dd(N);
		ps = new PolySet();
		ps->is2d = true;
		dxf_tesselate(ps, &dd, 0, true, 0);
	}

	if (N.dim == 3)
	{
		if (!N.p3.is_simple()) {
			PRINTF("WARNING: Result of render() isn't valid 2-manifold! Modify your design..");
			return NULL;
		}

		ps = new PolySet();
		
		CGAL_Polyhedron P;
		N.p3.convert_to_Polyhedron(P);

		typedef CGAL_Polyhedron::Vertex Vertex;
		typedef CGAL_Polyhedron::Vertex_const_iterator VCI;
		typedef CGAL_Polyhedron::Facet_const_iterator FCI;
		typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

		for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
			HFCC hc = fi->facet_begin();
			HFCC hc_end = hc;
			ps->append_poly();
			do {
				Vertex v = *VCI((hc++)->vertex());
				double x = CGAL::to_double(v.point().x());
				double y = CGAL::to_double(v.point().y());
				double z = CGAL::to_double(v.point().z());
				ps->append_vertex(x, y, z);
			} while (hc != hc_end);
		}
	}

	if (ps)
	{
		ps->convexity = convexity;
		PolySet::ps_cache.insert(key, new PolySetPtr(ps->link()));

		CSGTerm *term = new CSGTerm(ps, m, QString("n%1").arg(idx));
		if (modinst->tag_highlight && highlights)
			highlights->append(term->link());
		if (modinst->tag_background && background) {
			background->append(term);
			return NULL;
		}
		return term;
	}

	return NULL;
}

#else

CSGTerm *RenderNode::render_csg_term(double m[16], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	CSGTerm *t1 = NULL;
	PRINT("WARNING: Found render() statement but compiled without CGAL support!");
	foreach(AbstractNode * v, children) {
		CSGTerm *t2 = v->render_csg_term(m, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
		}
	}
	if (modinst->tag_highlight && highlights)
		highlights->append(t1->link());
	if (t1 && modinst->tag_background && background) {
		background->append(t1);
		return NULL;
	}
	return t1;
}

#endif

QString RenderNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text = indent + QString("n%1: ").arg(idx) + QString("render(convexity = %1) {\n").arg(QString::number(convexity));
		foreach (AbstractNode *v, children)
			text += v->dump(indent + QString("\t"));
		((AbstractNode*)this)->dump_cache = text + indent + "}\n";
	}
	return dump_cache;
}

