#include "CSGTermEvaluator.h"
#include "visitor.h"
#include "state.h"
#include "csgterm.h"
#include "module.h"
#include "csgnode.h"
#include "transformnode.h"
#include "rendernode.h"
#include "printutils.h"

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>

/*!
	\class CSGTermEvaluator

	A visitor responsible for creating a tree of CSGTerm nodes used for rendering
	with OpenCSG.
*/

CSGTerm *CSGTermEvaluator::evaluateCSGTerm(const AbstractNode &node, 
																					 std::vector<CSGTerm*> &highlights, 
																					 std::vector<CSGTerm*> &background)
{
	Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
	evaluate.execute();
	highlights = this->highlights;
	background = this->background;
	return this->stored_term[node.index()];
}

void CSGTermEvaluator::applyToChildren(const AbstractNode &node, CSGTermEvaluator::CsgOp op)
{
	CSGTerm *t1 = NULL;
	for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
			 iter != this->visitedchildren[node.index()].end();
			 iter++) {
		const AbstractNode *chnode = *iter;
		CSGTerm *t2 = this->stored_term[chnode->index()];
		this->stored_term.erase(chnode->index());
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (op == CSGT_UNION) {
				t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
			} else if (op == CSGT_DIFFERENCE) {
				t1 = new CSGTerm(CSGTerm::TYPE_DIFFERENCE, t1, t2);
			} else if (op == CSGT_INTERSECTION) {
				t1 = new CSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			}
		}
	}
	if (t1 && node.modinst->tag_highlight) {
		this->highlights.push_back(t1->link());
	}
	if (t1 && node.modinst->tag_background) {
		this->background.push_back(t1);
		t1 = NULL; // don't propagate background tagged nodes
	}
	this->stored_term[node.index()] = t1;
}

Response CSGTermEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_INTERSECTION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

static CSGTerm *evaluate_csg_term_from_ps(const double m[20], 
																				vector<CSGTerm*> &highlights, 
																				vector<CSGTerm*> &background, 
																				PolySet *ps, 
																				const ModuleInstantiation *modinst, 
																				const AbstractPolyNode &node)
{
	CSGTerm *t = new CSGTerm(ps, m, QString("%1%2").arg(node.name().c_str()).arg(node.index()));
	if (modinst->tag_highlight)
		highlights.push_back(t->link());
	if (modinst->tag_background) {
		background.push_back(t);
		return NULL;
	}
	return t;
}

Response CSGTermEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPostfix()) {
		CSGTerm *t1 = NULL;
		PolySet *ps = node.evaluate_polyset(AbstractPolyNode::RENDER_OPENCSG, this->psevaluator);
		if (ps) {
			t1 = evaluate_csg_term_from_ps(state.matrix(), this->highlights, this->background, 
																	 ps, node.modinst, node);
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPostfix()) {
		CsgOp op;
		switch (node.type) {
		case CSG_TYPE_UNION:
			op = CSGT_UNION;
			break;
		case CSG_TYPE_DIFFERENCE:
			op = CSGT_DIFFERENCE;
			break;
		case CSG_TYPE_INTERSECTION:
			op = CSGT_INTERSECTION;
			break;
		}
		applyToChildren(node, op);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix()) {
		double m[20];
		
		for (int i = 0; i < 16; i++)
		{
			int c_row = i%4;
			int m_col = i/4;
			m[i] = 0;
			for (int j = 0; j < 4; j++) {
				m[i] += state.matrix()[c_row + j*4] * node.matrix[m_col*4 + j];
			}
		}
		
		for (int i = 16; i < 20; i++) {
			m[i] = node.matrix[i] < 0 ? state.matrix()[i] : node.matrix[i];
		}

		state.setMatrix(m);
	}
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

// FIXME: Find out how to best call into CGAL from this visitor
Response CSGTermEvaluator::visit(State &state, const RenderNode &node)
{
	PRINT("WARNING: Found render() statement but compiled without CGAL support!");
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CSGTermEvaluator::addToParent(const State &state, const AbstractNode &node)
{
	assert(state.isPostfix());
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(&node);
	}
}


#if 0

// FIXME: #ifdef ENABLE_CGAL
#if 0
CSGTerm *CgaladvNode::evaluate_csg_term(double m[20], QVector<CSGTerm*> &highlights, QVector<CSGTerm*> &background) const
{
	if (type == MINKOWSKI)
		return evaluate_csg_term_from_nef(m, highlights, background, "minkowski", this->convexity);

	if (type == GLIDE)
		return evaluate_csg_term_from_nef(m, highlights, background, "glide", this->convexity);

	if (type == SUBDIV)
		return evaluate_csg_term_from_nef(m, highlights, background, "subdiv", this->convexity);

	if (type == HULL)
		return evaluate_csg_term_from_nef(m, highlights, background, "hull", this->convexity);

	return NULL;
}

#else // ENABLE_CGAL

CSGTerm *CgaladvNode::evaluate_csg_term(double m[20], QVector<CSGTerm*> &highlights, QVector<CSGTerm*> &background) const
{
	PRINT("WARNING: Found minkowski(), glide(), subdiv() or hull() statement but compiled without CGAL support!");
	return NULL;
}

#endif // ENABLE_CGAL



// FIXME: #ifdef ENABLE_CGAL
#if 0
CSGTerm *AbstractNode::evaluate_csg_term_from_nef(double m[20], QVector<CSGTerm*> &highlights, QVector<CSGTerm*> &background, const char *statement, int convexity) const
{
	QString key = mk_cache_id();
	if (PolySet::ps_cache.contains(key)) {
		PRINT(PolySet::ps_cache[key]->msg);
		return AbstractPolyNode::evaluate_csg_term_from_ps(m, highlights, background,
				PolySet::ps_cache[key]->ps->link(), modinst, idx);
	}

	print_messages_push();
	CGAL_Nef_polyhedron N;

	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id))
	{
		PRINT(cgal_nef_cache[cache_id]->msg);
		N = cgal_nef_cache[cache_id]->N;
	}
	else
	{
		PRINTF_NOCACHE("Processing uncached %s statement...", statement);
		// PRINTA("Cache ID: %1", cache_id);
		QApplication::processEvents();

		QTime t;
		t.start();

		N = this->evaluateCSGMesh();

		int s = t.elapsed() / 1000;
		PRINTF_NOCACHE("..processing time: %d hours, %d minutes, %d seconds", s / (60*60), (s / 60) % 60, s % 60);
	}

	PolySet *ps = NULL;

	if (N.dim == 2)
	{
		DxfData dd(N);
		ps = new PolySet();
		ps->is2d = true;
		dxf_tesselate(ps, &dd, 0, true, false, 0);
		dxf_border_to_ps(ps, &dd);
	}

	if (N.dim == 3)
	{
		if (!N.p3.is_simple()) {
			PRINTF("WARNING: Result of %s() isn't valid 2-manifold! Modify your design..", statement);
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
		PolySet::ps_cache.insert(key, new PolySet::ps_cache_entry(ps->link()));

		CSGTerm *term = new CSGTerm(ps, m, QString("n%1").arg(idx));
		if (modinst->tag_highlight)
			highlights.push_back(term->link());
		if (modinst->tag_background) {
			background.push_back(term);
			return NULL;
		}
		return term;
	}
	print_messages_pop();

	return NULL;
}

CSGTerm *RenderNode::evaluate_csg_term(double m[20], QVector<CSGTerm*> &highlights, QVector<CSGTerm*> &background) const
{
	return evaluate_csg_term_from_nef(m, highlights, background, "render", this->convexity);
}

#else

CSGTerm *RenderNode::evaluate_csg_term(double m[20], QVector<CSGTerm*> &highlights, QVector<CSGTerm*> &background) const
{
	CSGTerm *t1 = NULL;
	PRINT("WARNING: Found render() statement but compiled without CGAL support!");
	foreach(AbstractNode * v, children) {
		CSGTerm *t2 = v->evaluate_csg_term(m, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
		}
	}
	if (modinst->tag_highlight)
		highlights.push_back(t1->link());
	if (t1 && modinst->tag_background) {
		background.push_back(t1);
		return NULL;
	}
	return t1;
}

#endif



#endif

