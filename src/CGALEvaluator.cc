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
#include "Tree.h"

#include "CGALCache.h"
#include "cgal.h"
#include "cgalutils.h"

#ifdef NDEBUG
#define PREV_NDEBUG NDEBUG
#undef NDEBUG
#endif
#include <CGAL/convex_hull_3.h>
#ifdef PREV_NDEBUG
#define NDEBUG PREV_NDEBUG
#endif

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <map>

CGAL_Nef_polyhedron CGALEvaluator::evaluateCGALMesh(const AbstractNode &node)
{
	if (!isCached(node)) {
		Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
		evaluate.execute();
		return this->root;
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

	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
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
	catch (const CGAL::Failure_exception &e) {
		// union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
		std::string opstr = op == CGE_UNION ? "union" : op == CGE_INTERSECTION ? "intersection" : op == CGE_DIFFERENCE ? "difference" : op == CGE_MINKOWSKI ? "minkowski" : "UNKNOWN";
		PRINTB("CGAL error in CGAL_Nef_polyhedron's %s operator: %s", opstr % e.what());

		// Errors can result in corrupt polyhedrons, so put back the old one
		target = src;
	}
	CGAL::set_error_behaviour(old_behaviour);
}

/*!
*/
CGAL_Nef_polyhedron CGALEvaluator::applyToChildren(const AbstractNode &node, CGALEvaluator::CsgOp op)
{
	CGAL_Nef_polyhedron N;
	BOOST_FOREACH(const ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const CGAL_Nef_polyhedron &chN = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the 
    // cache could have been modified before we reach this point due to a large
    // sibling object. 
		if (!isCached(*chnode)) {
			CGALCache::instance()->insert(this->tree.getIdString(*chnode), chN);
		}
		if (N.empty()) N = chN.copy();
		else process(N, chN, op);

		chnode->progress_report();
	}
	return N;
}

CGAL_Nef_polyhedron CGALEvaluator::applyHull(const CgaladvNode &node)
{
	CGAL_Nef_polyhedron N;
	std::list<CGAL_Nef_polyhedron2*> polys;
	std::list<CGAL_Nef_polyhedron2::Point> points2d;
	std::list<CGAL_Polyhedron::Vertex::Point_3> points3d;
	int dim = 0;
	BOOST_FOREACH(const ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const CGAL_Nef_polyhedron &chN = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;
		if (dim == 0) {
			dim = chN.dim;
		}
		else if (dim != chN.dim) {
			PRINT("WARNING: hull() does not support mixing 2D and 3D objects.");
			continue;
		}
		if (dim == 2) {
			CGAL_Nef_polyhedron2::Explorer explorer = chN.p2->explorer();
			BOOST_FOREACH(const CGAL_Nef_polyhedron2::Explorer::Vertex &vh, 
										std::make_pair(explorer.vertices_begin(), explorer.vertices_end())) {
				if (explorer.is_standard(&vh)) {
					points2d.push_back(explorer.point(&vh));
				}
			}
		}
		else if (dim == 3) {
			CGAL_Polyhedron P;
			if (!chN.p3->is_simple()) {
				PRINT("Hull() currently requires a valid 2-manifold. Please modify your design. See http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export");
			}
			else {
				chN.p3->convert_to_Polyhedron(P);
				std::transform(P.vertices_begin(), P.vertices_end(), std::back_inserter(points3d), 
											 boost::bind(static_cast<const CGAL_Polyhedron::Vertex::Point_3&(CGAL_Polyhedron::Vertex::*)() const>(&CGAL_Polyhedron::Vertex::point), _1));
			}
		}
		chnode->progress_report();
	}
	
	if (dim == 2) {
		std::list<CGAL_Nef_polyhedron2::Point> result;
		CGAL::convex_hull_2(points2d.begin(), points2d.end(),std:: back_inserter(result));
		N = CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron2(result.begin(), result.end(), 
																										 CGAL_Nef_polyhedron2::INCLUDED));
	}
	else if (dim == 3) {
		CGAL_Polyhedron P;
		if (points3d.size()>3)
			CGAL::convex_hull_3(points3d.begin(), points3d.end(), P);
		N = CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(P));
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
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) N = applyToChildren(node, CGE_UNION);
		else N = CGALCache::instance()->get(this->tree.getIdString(node));
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) N = applyToChildren(node, CGE_INTERSECTION);
		else N = CGALCache::instance()->get(this->tree.getIdString(node));
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
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
			N = applyToChildren(node, op);
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			// First union all children
			N = applyToChildren(node, CGE_UNION);

			if ( matrix_contains_infinity( node.matrix ) || matrix_contains_nan( node.matrix ) ) {
				// due to the way parse/eval works we can't currently distinguish between NaN and Inf
				PRINT("Warning: Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
				N.reset();
			}

			// Then apply transform
			// If there is no geometry under the transform, N will be empty and of dim 0,
			// just just silently ignore such nodes
			if (N.dim == 2) {
				// Unfortunately CGAL provides no transform method for CGAL_Nef_polyhedron2
				// objects. So we convert in to our internal 2d data format, transform it,
				// tesselate it and create a new CGAL_Nef_polyhedron2 from it.. What a hack!
				
				Eigen::Matrix2f testmat;
				testmat << node.matrix(0,0), node.matrix(0,1), node.matrix(1,0), node.matrix(1,1);
				if (testmat.determinant() == 0) {
					PRINT("Warning: Scaling a 2D object with 0 - removing object");
					N.reset();
				}
				else {
					CGAL_Aff_transformation2 t(
						node.matrix(0,0), node.matrix(0,1), node.matrix(0,3),
						node.matrix(1,0), node.matrix(1,1), node.matrix(1,3), node.matrix(3,3));
					
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
			}
			else if (N.dim == 3) {
				if (node.matrix.matrix().determinant() == 0) {
					PRINT("Warning: Scaling a 3D object with 0 - removing object");
					N.reset();
				}
				else {
					CGAL_Aff_transformation t(
						node.matrix(0,0), node.matrix(0,1), node.matrix(0,2), node.matrix(0,3),
						node.matrix(1,0), node.matrix(1,1), node.matrix(1,2), node.matrix(1,3),
						node.matrix(2,0), node.matrix(2,1), node.matrix(2,2), node.matrix(2,3), node.matrix(3,3));
					N.p3->transform(t);
				}
			}
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			// Apply polyset operation
			shared_ptr<PolySet> ps = this->psevaluator.getPolySet(node, false);
			if (ps) {
				N = evaluateCGALMesh(*ps);
//				print_messages_pop();
				node.progress_report();
			}
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
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
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CGALEvaluator::addToParent(const State &state, const AbstractNode &node, const CGAL_Nef_polyhedron &N)
{
	assert(state.isPostfix());
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(std::make_pair(&node, N));
	}
	else {
		// Root node, insert into cache
		if (!isCached(node)) {
			if (!CGALCache::instance()->insert(this->tree.getIdString(node), N)) {
				PRINT("WARNING: CGAL Evaluator: Root node didn't fit into cache");
			}
		}
		this->root = N;
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
			std::map<std::pair<int,int>, std::pair<int,int> > edge_to_poly;
			std::map<int, CGAL_Nef_polyhedron2::Point> points;
			typedef std::map<int, std::vector<int> > PolygonMap;
			PolygonMap polygons;
			int poly_n;

			void add_edges(int pn)
			{
				for (unsigned int j = 1; j <= this->polygons[pn].size(); j++) {
					int a = this->polygons[pn][j-1];
					int b = this->polygons[pn][j % this->polygons[pn].size()];
					if (a > b) { a = a^b; b = a^b; a = a^b; }
					if (this->edge_to_poly[std::pair<int,int>(a, b)].first == 0)
						this->edge_to_poly[std::pair<int,int>(a, b)].first = pn;
					else if (this->edge_to_poly[std::pair<int,int>(a, b)].second == 0)
						this->edge_to_poly[std::pair<int,int>(a, b)].second = pn;
					else
						abort();
				}
			}

			void del_poly(int pn)
			{
				for (unsigned int j = 1; j <= this->polygons[pn].size(); j++) {
					int a = this->polygons[pn][j-1];
					int b = this->polygons[pn][j % this->polygons[pn].size()];
					if (a > b) { a = a^b; b = a^b; a = a^b; }
					if (this->edge_to_poly[std::pair<int,int>(a, b)].first == pn)
						this->edge_to_poly[std::pair<int,int>(a, b)].first = 0;
					if (this->edge_to_poly[std::pair<int,int>(a, b)].second == pn)
						this->edge_to_poly[std::pair<int,int>(a, b)].second = 0;
				}
				this->polygons.erase(pn);
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
							std::vector<int> &poly = this->polygons[this->poly_n];
							if (std::find(poly.begin(), poly.end(), idx) == poly.end()) {
								poly.push_back(this->grid.data(x, y));
							}
						} else {
							this->grid.align(x, y) = point_n;
							this->polygons[this->poly_n].push_back(point_n);
							this->points[point_n] = CGAL_Nef_polyhedron2::Point(x, y);
							point_n++;
						}
					}
					if (this->polygons[this->poly_n].size() >= 3) {
						add_edges(this->poly_n);
						this->poly_n++;
					}
					else {
						this->polygons.erase(this->poly_n);
					}
				}
			}

			int merge(int p1, int p1e, int p2, int p2e)
			{
				for (unsigned int i = 1; i < this->polygons[p1].size(); i++) {
					int j = (p1e + i) % this->polygons[p1].size();
					this->polygons[this->poly_n].push_back(this->polygons[p1][j]);
				}
				for (unsigned int i = 1; i < this->polygons[p2].size(); i++) {
					int j = (p2e + i) % this->polygons[p2].size();
					this->polygons[this->poly_n].push_back(this->polygons[p2][j]);
				}
				del_poly(p1);
				del_poly(p2);
				add_edges(this->poly_n);
				return this->poly_n++;
			}

			void reduce()
			{
				std::deque<int> work_queue;
				BOOST_FOREACH(const PolygonMap::value_type &i, polygons) {
					work_queue.push_back(i.first);
				}
				while (!work_queue.empty()) {
					int poly1_n = work_queue.front();
					work_queue.pop_front();
					if (this->polygons.find(poly1_n) == this->polygons.end()) continue;
					for (unsigned int j = 1; j <= this->polygons[poly1_n].size(); j++) {
						int a = this->polygons[poly1_n][j-1];
						int b = this->polygons[poly1_n][j % this->polygons[poly1_n].size()];
						if (a > b) { a = a^b; b = a^b; a = a^b; }
						if (this->edge_to_poly[std::pair<int,int>(a, b)].first != 0 &&
								this->edge_to_poly[std::pair<int,int>(a, b)].second != 0) {
							int poly2_n = this->edge_to_poly[std::pair<int,int>(a, b)].first +
									this->edge_to_poly[std::pair<int,int>(a, b)].second - poly1_n;
							int poly2_edge = -1;
							for (unsigned int k = 1; k <= this->polygons[poly2_n].size(); k++) {
								int c = this->polygons[poly2_n][k-1];
								int d = this->polygons[poly2_n][k % this->polygons[poly2_n].size()];
								if (c > d) { c = c^d; d = c^d; c = c^d; }
								if (a == c && b == d) {
									poly2_edge = k-1;
									continue;
								}
								int poly3_n = this->edge_to_poly[std::pair<int,int>(c, d)].first +
										this->edge_to_poly[std::pair<int,int>(c, d)].second - poly2_n;
								if (poly3_n < 0)
									continue;
								if (poly3_n == poly1_n)
									goto next_poly1_edge;
							}
							work_queue.push_back(merge(poly1_n, j-1, poly2_n, poly2_edge));
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

				BOOST_FOREACH(const PolygonMap::value_type &i, polygons) {
					std::list<CGAL_Nef_polyhedron2::Point> plist;
					for (unsigned int j = 0; j < i.second.size(); j++) {
						int p = i.second[j];
						plist.push_back(points[p]);
					}
					*N += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
				}

				return N;
			}
		};

		PolyReducer pr(ps);
		pr.reduce();
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
		CGAL_Nef_polyhedron3 *N = NULL;
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			// FIXME: Are we leaking memory for the CGAL_Polyhedron object?
			CGAL_Polyhedron *P = createPolyhedronFromPolySet(ps);
			if (P) {
				N = new CGAL_Nef_polyhedron3(*P);
			}
		}
		catch (const CGAL::Assertion_exception &e) {
			PRINTB("CGAL error in CGAL_Nef_polyhedron3(): %s", e.what());
		}
		CGAL::set_error_behaviour(old_behaviour);
		return CGAL_Nef_polyhedron(N);
	}
	return CGAL_Nef_polyhedron();
}
