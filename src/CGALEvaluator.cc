#include "CGALCache.h"
#include "CGALEvaluator.h"
#include "visitor.h"
#include "state.h"
#include "module.h" // FIXME: Temporarily for ModuleInstantiation
#include "printutils.h"

#include "csgnode.h"
#include "cgaladvnode.h"
#include "transformnode.h"
#include "polyset.h"
#include "dxfdata.h"
#include "dxftess.h"

#include "CGALCache.h"
#include "cgal.h"
#include "cgalutils.h"
#include <CGAL/assertions_behaviour.h>
#include <CGAL/exceptions.h>

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <QRegExp>

CGAL_Nef_polyhedron CGALEvaluator::evaluateCGALMesh(const AbstractNode &node)
{
	if (!isCached(node)) {
		Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
		evaluate.execute();
		assert(isCached(node));
	}
	return CGALCache::instance()->get(this->tree.getIdString(node));
}

bool CGALEvaluator::isCached(const AbstractNode &node) const
{
	return CGALCache::instance()->contains(this->tree.getIdString(node));
}

/*!
	Modifies target by applying op to target and src:
	target = target [op] src
 */
void CGALEvaluator::process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CGALEvaluator::CsgOp op)
{
 	if (target.dim != 2 && target.dim != 3) {
 		assert(false && "Dimension of Nef polyhedron must be 2 or 3");
 	}
	if (src.empty()) return; // Empty polyhedron. This can happen for e.g. square([0,0])
	if (target.dim != src.dim) return; // If someone tries to e.g. union 2d and 3d objects

	switch (op) {
	case CGE_UNION:
		target += src;
		break;
	case CGE_INTERSECTION:
		target *= src;
		break;
	case CGE_DIFFERENCE:
		target -= src;
		break;
	case CGE_MINKOWSKI:
		target.minkowski(src);
		break;
	}
}

/*!
*/
CGAL_Nef_polyhedron CGALEvaluator::applyToChildren(const AbstractNode &node, CGALEvaluator::CsgOp op)
{
	CGAL_Nef_polyhedron N;
	if (this->visitedchildren[node.index()].size() > 0) {
		for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
				 iter != this->visitedchildren[node.index()].end();
				 iter++) {
			const AbstractNode *chnode = iter->first;
			const string &chcacheid = iter->second;
			// FIXME: Don't use deep access to modinst members
			if (chnode->modinst->tag_background) continue;
			assert(isCached(*chnode));
			if (N.empty()) {
				N = CGALCache::instance()->get(chcacheid).copy();
			} else {
				process(N, CGALCache::instance()->get(chcacheid), op);
			}
			chnode->progress_report();
		}
	}
	return N;
}

extern CGAL_Nef_polyhedron2 *convexhull2(std::list<CGAL_Nef_polyhedron2*> a);

CGAL_Nef_polyhedron CGALEvaluator::applyHull(const CgaladvNode &node)
{
	CGAL_Nef_polyhedron N;
	if (this->visitedchildren[node.index()].size() > 0) {
		std::list<CGAL_Nef_polyhedron2*> polys;
		bool all2d = true;
		for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
				 iter != this->visitedchildren[node.index()].end();
				 iter++) {
			const AbstractNode *chnode = iter->first;
			const string &chcacheid = iter->second;
			// FIXME: Don't use deep access to modinst members
			if (chnode->modinst->tag_background) continue;
			assert(isCached(*chnode));
			const CGAL_Nef_polyhedron &ch = CGALCache::instance()->get(chcacheid);
			if (ch.dim == 2) {
		    polys.push_back(ch.p2.get());
			}
			else if (ch.dim == 3) {
				PRINT("WARNING: hull() is not implemented yet for 3D objects!");
		    all2d = false;
			}
			chnode->progress_report();
		}

		if (all2d) {
			N = CGAL_Nef_polyhedron(convexhull2(polys));
		}
	}
	return N;
}

/*
	Typical visitor behavior:
	o In prefix: Check if we're cached -> prune
	o In postfix: Check if we're cached -> don't apply operator to children
	o In postfix: addToParent()
 */

Response CGALEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			CGAL_Nef_polyhedron N = applyToChildren(node, CGE_UNION);
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			CGAL_Nef_polyhedron N = applyToChildren(node, CGE_INTERSECTION);
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			CGALEvaluator::CsgOp op;
			switch (node.type) {
			case CSG_TYPE_UNION:
				op = CGE_UNION;
				break;
			case CSG_TYPE_DIFFERENCE:
				op = CGE_DIFFERENCE;
				break;
			case CSG_TYPE_INTERSECTION:
				op = CGE_INTERSECTION;
				break;
			default:
				assert(false);
			}
			CGAL_Nef_polyhedron N = applyToChildren(node, op);
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			// First union all children
			CGAL_Nef_polyhedron N = applyToChildren(node, CGE_UNION);

			// Then apply transform
			// If there is no geometry under the transform, N will be empty and of dim 0,
			// just just silently ignore such nodes
			if (N.dim == 2) {
				// Unfortunately CGAL provides no transform method for CGAL_Nef_polyhedron2
				// objects. So we convert in to our internal 2d data format, transform it,
				// tesselate it and create a new CGAL_Nef_polyhedron2 from it.. What a hack!
				
				CGAL_Aff_transformation2 t(
					node.matrix[0], node.matrix[4], node.matrix[12],
					node.matrix[1], node.matrix[5], node.matrix[13], node.matrix[15]);
				
				DxfData *dd = N.convertToDxfData();
				for (size_t i=0; i < dd->points.size(); i++) {
					CGAL_Kernel2::Point_2 p = CGAL_Kernel2::Point_2(dd->points[i][0], dd->points[i][1]);
					p = t.transform(p);
					dd->points[i][0] = to_double(p.x());
					dd->points[i][1] = to_double(p.y());
				}
				
				PolySet ps;
				ps.is2d = true;
				dxf_tesselate(&ps, *dd, 0, true, false, 0);
				
				N = evaluateCGALMesh(ps);
				delete dd;
			}
			else if (N.dim == 3) {
				CGAL_Aff_transformation t(
					node.matrix[0], node.matrix[4], node.matrix[ 8], node.matrix[12],
					node.matrix[1], node.matrix[5], node.matrix[ 9], node.matrix[13],
					node.matrix[2], node.matrix[6], node.matrix[10], node.matrix[14], node.matrix[15]);
				N.p3->transform(t);
			}
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			// Apply polyset operation
			shared_ptr<PolySet> ps = this->psevaluator.getPolySet(node, false);
			CGAL_Nef_polyhedron N;
			if (ps) {
				N = evaluateCGALMesh(*ps);
//				print_messages_pop();
				node.progress_report();
			}
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			CGAL_Nef_polyhedron N;
			CGALEvaluator::CsgOp op;
			switch (node.type) {
			case MINKOWSKI:
				op = CGE_MINKOWSKI;
				N = applyToChildren(node, op);
				break;
			case GLIDE:
				PRINT("WARNING: glide() is not implemented yet!");
				return PruneTraversal;
				break;
			case SUBDIV:
				PRINT("WARNING: subdiv() is not implemented yet!");
				return PruneTraversal;
				break;
			case HULL:
				N = applyHull(node);
				break;
			}
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CGALEvaluator::addToParent(const State &state, const AbstractNode &node)
{
	assert(state.isPostfix());
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(std::make_pair(&node, this->tree.getIdString(node)));
	}
}

CGAL_Nef_polyhedron CGALEvaluator::evaluateCGALMesh(const PolySet &ps)
{
	if (ps.empty()) return CGAL_Nef_polyhedron();

	if (ps.is2d)
	{
#if 0
		// This version of the code causes problems in some cases.
		// Example testcase: import_dxf("testdata/polygon8.dxf");
		//
		typedef std::list<CGAL_Nef_polyhedron2::Point> point_list_t;
		typedef point_list_t::iterator point_list_it;
		std::list< point_list_t > pdata_point_lists;
		std::list < std::pair < point_list_it, point_list_it > > pdata;
		Grid2d<CGAL_Nef_polyhedron2::Point> grid(GRID_COARSE);

		for (int i = 0; i < ps.polygons.size(); i++) {
			pdata_point_lists.push_back(point_list_t());
			for (int j = 0; j < ps.polygons[i].size(); j++) {
				double x = ps.polygons[i][j].x;
				double y = ps.polygons[i][j].y;
				CGAL_Nef_polyhedron2::Point p;
				if (grid.has(x, y)) {
					p = grid.data(x, y);
				} else {
					p = CGAL_Nef_polyhedron2::Point(x, y);
					grid.data(x, y) = p;
				}
				pdata_point_lists.back().push_back(p);
			}
			pdata.push_back(std::make_pair(pdata_point_lists.back().begin(),
					pdata_point_lists.back().end()));
		}

		CGAL_Nef_polyhedron2 N(pdata.begin(), pdata.end(), CGAL_Nef_polyhedron2::POLYGONS);
		return CGAL_Nef_polyhedron(N);
#endif
#if 0
		// This version of the code works fine but is pretty slow.
		//
		CGAL_Nef_polyhedron2 N;
		Grid2d<CGAL_Nef_polyhedron2::Point> grid(GRID_COARSE);

		for (int i = 0; i < ps.polygons.size(); i++) {
			std::list<CGAL_Nef_polyhedron2::Point> plist;
			for (int j = 0; j < ps.polygons[i].size(); j++) {
				double x = ps.polygons[i][j].x;
				double y = ps.polygons[i][j].y;
				CGAL_Nef_polyhedron2::Point p;
				if (grid.has(x, y)) {
					p = grid.data(x, y);
				} else {
					p = CGAL_Nef_polyhedron2::Point(x, y);
					grid.data(x, y) = p;
				}
				plist.push_back(p);
			}
			N += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
		}

		return CGAL_Nef_polyhedron(N);
#endif
#if 1
		// This version of the code does essentially the same thing as the 2nd
		// version but merges some triangles before sending them to CGAL. This adds
		// complexity but speeds up things..
		//
		struct PolyReducer
		{
			Grid2d<int> grid;
			QHash< QPair<int,int>, QPair<int,int> > egde_to_poly;
			QHash< int, CGAL_Nef_polyhedron2::Point > points;
			QHash< int, QList<int> > polygons;
			int poly_n;

			void add_edges(int pn)
			{
				for (int j = 1; j <= this->polygons[pn].size(); j++) {
					int a = this->polygons[pn][j-1];
					int b = this->polygons[pn][j % this->polygons[pn].size()];
					if (a > b) { a = a^b; b = a^b; a = a^b; }
					if (this->egde_to_poly[QPair<int,int>(a, b)].first == 0)
						this->egde_to_poly[QPair<int,int>(a, b)].first = pn;
					else if (this->egde_to_poly[QPair<int,int>(a, b)].second == 0)
						this->egde_to_poly[QPair<int,int>(a, b)].second = pn;
					else
						abort();
				}
			}

			void del_poly(int pn)
			{
				for (int j = 1; j <= this->polygons[pn].size(); j++) {
					int a = this->polygons[pn][j-1];
					int b = this->polygons[pn][j % this->polygons[pn].size()];
					if (a > b) { a = a^b; b = a^b; a = a^b; }
					if (this->egde_to_poly[QPair<int,int>(a, b)].first == pn)
						this->egde_to_poly[QPair<int,int>(a, b)].first = 0;
					if (this->egde_to_poly[QPair<int,int>(a, b)].second == pn)
						this->egde_to_poly[QPair<int,int>(a, b)].second = 0;
				}
				this->polygons.remove(pn);
			}

			PolyReducer(const PolySet &ps) : grid(GRID_COARSE), poly_n(1)
			{
				int point_n = 1;
				for (size_t i = 0; i < ps.polygons.size(); i++) {
					for (size_t j = 0; j < ps.polygons[i].size(); j++) {
						double x = ps.polygons[i][j][0];
						double y = ps.polygons[i][j][1];
						if (this->grid.has(x, y)) {
							int idx = this->grid.data(x, y);
							// Filter away two vertices with the same index (due to grid)
							// This could be done in a more general way, but we'd rather redo the entire
							// grid concept instead.
							if (this->polygons[this->poly_n].indexOf(idx) == -1) {
								this->polygons[this->poly_n].append(this->grid.data(x, y));
							}
						} else {
							this->grid.align(x, y) = point_n;
							this->polygons[this->poly_n].append(point_n);
							this->points[point_n] = CGAL_Nef_polyhedron2::Point(x, y);
							point_n++;
						}
					}
					if (this->polygons[this->poly_n].size() >= 3) {
						add_edges(this->poly_n);
						this->poly_n++;
					}
					else {
						this->polygons.remove(this->poly_n);
					}
				}
			}

			int merge(int p1, int p1e, int p2, int p2e)
			{
				for (int i = 1; i < this->polygons[p1].size(); i++) {
					int j = (p1e + i) % this->polygons[p1].size();
					this->polygons[this->poly_n].append(this->polygons[p1][j]);
				}
				for (int i = 1; i < this->polygons[p2].size(); i++) {
					int j = (p2e + i) % this->polygons[p2].size();
					this->polygons[this->poly_n].append(this->polygons[p2][j]);
				}
				del_poly(p1);
				del_poly(p2);
				add_edges(this->poly_n);
				return this->poly_n++;
			}

			void reduce()
			{
				QList<int> work_queue;
				QHashIterator< int, QList<int> > it(polygons);
				while (it.hasNext()) {
					it.next();
					work_queue.append(it.key());
				}
				while (!work_queue.isEmpty()) {
					int poly1_n = work_queue.first();
					work_queue.removeFirst();
					if (!this->polygons.contains(poly1_n))
						continue;
					for (int j = 1; j <= this->polygons[poly1_n].size(); j++) {
						int a = this->polygons[poly1_n][j-1];
						int b = this->polygons[poly1_n][j % this->polygons[poly1_n].size()];
						if (a > b) { a = a^b; b = a^b; a = a^b; }
						if (this->egde_to_poly[QPair<int,int>(a, b)].first != 0 &&
								this->egde_to_poly[QPair<int,int>(a, b)].second != 0) {
							int poly2_n = this->egde_to_poly[QPair<int,int>(a, b)].first +
									this->egde_to_poly[QPair<int,int>(a, b)].second - poly1_n;
							int poly2_edge = -1;
							for (int k = 1; k <= this->polygons[poly2_n].size(); k++) {
								int c = this->polygons[poly2_n][k-1];
								int d = this->polygons[poly2_n][k % this->polygons[poly2_n].size()];
								if (c > d) { c = c^d; d = c^d; c = c^d; }
								if (a == c && b == d) {
									poly2_edge = k-1;
									continue;
								}
								int poly3_n = this->egde_to_poly[QPair<int,int>(c, d)].first +
										this->egde_to_poly[QPair<int,int>(c, d)].second - poly2_n;
								if (poly3_n < 0)
									continue;
								if (poly3_n == poly1_n)
									goto next_poly1_edge;
							}
							work_queue.append(merge(poly1_n, j-1, poly2_n, poly2_edge));
							goto next_poly1;
						}
					next_poly1_edge:;
					}
				next_poly1:;
				}
			}

			CGAL_Nef_polyhedron2 *toNef()
			{
				CGAL_Nef_polyhedron2 *N = new CGAL_Nef_polyhedron2;

				QHashIterator< int, QList<int> > it(polygons);
				while (it.hasNext()) {
					it.next();
					std::list<CGAL_Nef_polyhedron2::Point> plist;
					for (int j = 0; j < it.value().size(); j++) {
						int p = it.value()[j];
						plist.push_back(points[p]);
					}
					*N += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
				}

				return N;
			}
		};

		PolyReducer pr(ps);
		PRINTF("Number of polygons before reduction: %d\n", pr.polygons.size());
		pr.reduce();
		PRINTF("Number of polygons after reduction: %d\n", pr.polygons.size());
		return CGAL_Nef_polyhedron(pr.toNef());
#endif
#if 0
		// This is another experimental version. I should run faster than the above,
		// is a lot simpler and has only one known weakness: Degenerate polygons, which
		// get repaired by GLUTess, might trigger a CGAL crash here. The only
		// known case for this is triangle-with-duplicate-vertex.dxf
		// FIXME: If we just did a projection, we need to recreate the border!
		if (ps.polygons.size() > 0) assert(ps.borders.size() > 0);
		CGAL_Nef_polyhedron2 N;
		Grid2d<CGAL_Nef_polyhedron2::Point> grid(GRID_COARSE);

		for (int i = 0; i < ps.borders.size(); i++) {
			std::list<CGAL_Nef_polyhedron2::Point> plist;
			for (int j = 0; j < ps.borders[i].size(); j++) {
				double x = ps.borders[i][j].x;
				double y = ps.borders[i][j].y;
				CGAL_Nef_polyhedron2::Point p;
				if (grid.has(x, y)) {
					p = grid.data(x, y);
				} else {
					p = CGAL_Nef_polyhedron2::Point(x, y);
					grid.data(x, y) = p;
				}
				plist.push_back(p);		
			}
			// FIXME: If a border (path) has a duplicate vertex in dxf,
			// the CGAL_Nef_polyhedron2 constructor will crash.
			N ^= CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
		}

		return CGAL_Nef_polyhedron(N);

#endif
	}
	else // not (this->is2d)
	{
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			CGAL_Polyhedron *P = createPolyhedronFromPolySet(ps);
			if (P) {
				CGAL_Nef_polyhedron3 *N = new CGAL_Nef_polyhedron3(*P);
				return CGAL_Nef_polyhedron(N);
			}
		}
		catch (CGAL::Assertion_exception e) {
			PRINTF("CGAL error in CGA_Nef_polyhedron3(): %s", e.what());
			CGAL::set_error_behaviour(old_behaviour);
			return CGAL_Nef_polyhedron();
		}
	}
	return CGAL_Nef_polyhedron();
}
