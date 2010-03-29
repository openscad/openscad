#include "CGALRenderer.h"
#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "state.h"
#include "nodecache.h"
#include "module.h" // FIXME: Temporarily for ModuleInstantiation

#include "csgnode.h"
#include "transformnode.h"
#include "polyset.h"
#include "dxfdata.h"
#include "dxftess.h"

#include <sstream>
#include <iostream>
#include <assert.h>
#include <QRegExp>

CGALRenderer *CGALRenderer::global_renderer = NULL;

CGAL_Nef_polyhedron CGALRenderer::renderCGALMesh(const AbstractNode &node)
{
	if (!isCached(node)) {
		Traverser render(*this, node, Traverser::PRE_AND_POSTFIX);
		render.execute();
		assert(isCached(node));
	}
	return this->cache[mk_cache_id(node)];
}

bool CGALRenderer::isCached(const AbstractNode &node) const
{
	return this->cache.contains(mk_cache_id(node));
}

/*!
	Modifies target by applying op to target and src:
	target = target [op] src
 */
void CGALRenderer::process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CsgOp op)
{
 	if (target.dim != 2 && target.dim != 3) {
 		assert(false && "Dimension of Nef polyhedron must be 2 or 3");
 	}

	if (target.dim == 2) {
		switch (op) {
		case UNION:
			target.p2 += src.p2;
			break;
		case INTERSECTION:
			target.p2 *= src.p2;
			break;
		case DIFFERENCE:
			target.p2 -= src.p2;
			break;
		case MINKOWSKI:
			target.p2 = minkowski2(target.p2, src.p2);
			break;
		}
	}
	else if (target.dim == 3) {
		switch (op) {
		case UNION:
			target.p3 += src.p3;
			break;
		case INTERSECTION:
			target.p3 *= src.p3;
			break;
		case DIFFERENCE:
			target.p3 -= src.p3;
			break;
		case MINKOWSKI:
			target.p3 = minkowski3(target.p3, src.p3);
			break;
		}
	}
}

/*!
	FIXME: Let caller insert into the cache since caller might modify the result
  (e.g. transform)
*/
void CGALRenderer::applyToChildren(const AbstractNode &node, CGALRenderer::CsgOp op)
{
	CGAL_Nef_polyhedron N;
	if (this->visitedchildren[node.index()].size() > 0) {
		bool first = true;
		for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
				 iter != this->visitedchildren[node.index()].end();
				 iter++) {
			const AbstractNode *chnode = iter->first;
			const QString &chcacheid = iter->second;
			// FIXME: Don't use deep access to modinst members
			if (chnode->modinst->tag_background) continue;
			assert(isCached(*chnode));
			if (first) {
				N = this->cache[chcacheid];
				// If the first child(ren) are empty (e.g. echo) nodes, 
				// ignore them (reset first flag)
 				if (N.dim != 0) first = false;
			} else {
				process(N, this->cache[chcacheid], op);
			}
			chnode->progress_report();
		}
	}
	QString cacheid = mk_cache_id(node);
	this->cache.insert(cacheid, N);
}

/*
	Typical visitor behavior:
	o In prefix: Check if we're cached -> prune
	o In postfix: Check if we're cached -> don't apply operator to children
	o In postfix: addToParent()
 */

Response CGALRenderer::visit(const State &state, const AbstractNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) applyToChildren(node, UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALRenderer::visit(const State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) applyToChildren(node, INTERSECTION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALRenderer::visit(const State &state, const CsgNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			CsgOp op;
			switch (node.type) {
			case CSG_TYPE_UNION:
				op = UNION;
				break;
			case CSG_TYPE_DIFFERENCE:
				op = DIFFERENCE;
				break;
			case CSG_TYPE_INTERSECTION:
				op = INTERSECTION;
				break;
			}
			applyToChildren(node, op);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CGALRenderer::visit(const State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			// First union all children
			applyToChildren(node, UNION);

			// Then apply transform
			QString cacheid = mk_cache_id(node);
			CGAL_Nef_polyhedron N = this->cache[cacheid];
			assert(N.dim >= 2 && N.dim <= 3);
			if (N.dim == 2) {
				// Unfortunately CGAL provides no transform method for CGAL_Nef_polyhedron2
				// objects. So we convert in to our internal 2d data format, transform it,
				// tesselate it and create a new CGAL_Nef_polyhedron2 from it.. What a hack!
				
				CGAL_Aff_transformation2 t(
					node.m[0], node.m[4], node.m[12],
					node.m[1], node.m[5], node.m[13], node.m[15]);
				
				DxfData dd(N);
				for (int i=0; i < dd.points.size(); i++) {
					CGAL_Kernel2::Point_2 p = CGAL_Kernel2::Point_2(dd.points[i].x, dd.points[i].y);
					p = t.transform(p);
					dd.points[i].x = to_double(p.x());
					dd.points[i].y = to_double(p.y());
				}
				
				PolySet ps;
				ps.is2d = true;
				dxf_tesselate(&ps, &dd, 0, true, false, 0);
				
				N = renderCGALMesh(ps);
				ps.refcount = 0;
			}
			else if (N.dim == 3) {
				CGAL_Aff_transformation t(
					node.m[0], node.m[4], node.m[ 8], node.m[12],
					node.m[1], node.m[5], node.m[ 9], node.m[13],
					node.m[2], node.m[6], node.m[10], node.m[14], node.m[15]);
				N.p3.transform(t);
			}
			this->cache.insert(cacheid, N);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

// FIXME: RenderNode: Union over children + some magic
// FIXME: CgaladvNode: Iterate over children. Special operation

// FIXME: Subtypes of AbstractPolyNode:
// ProjectionNode
// DxfLinearExtrudeNode
// DxfRotateExtrudeNode
// (SurfaceNode)
// (PrimitiveNode)
Response CGALRenderer::visit(const State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			// First union all children
			applyToChildren(node, UNION);

			// Then apply polyset operation
			PolySet *ps = node.render_polyset(AbstractPolyNode::RENDER_CGAL);
			try {
				CGAL_Nef_polyhedron N = renderCGALMesh(*ps);
//				print_messages_pop();
				node.progress_report();
				
				ps->unlink();
				QString cacheid = mk_cache_id(node);
				this->cache.insert(cacheid, N);
			}
			catch (...) { // Don't leak the PolySet on ProgressCancelException
				ps->unlink();
				throw;
			}
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CGALRenderer::addToParent(const State &state, const AbstractNode &node)
{
	assert(state.isPostfix());
	QString cacheid = mk_cache_id(node);
	this->visitedchildren.erase(node.index());
	if (!state.parent()) {
		this->root = &node;
	}
	else {
		this->visitedchildren[state.parent()->index()].push_back(std::make_pair(&node, cacheid));
	}
}

/*!
  Create a cache id of the entire tree under this node. This cache id
	is a non-whitespace plaintext of the evaluated scad tree and is used
	for lookup in cgal_nef_cache.
*/
QString CGALRenderer::mk_cache_id(const AbstractNode &node) const
{
	// FIXME: should we keep a cache of cache_id's to avoid recalculating this?
	// -> check how often we recalculate it.

	// FIXME: Get dump from dump cache
	// FIXME: assert that cache contains node
	QString cache_id = QString::fromStdString(this->dumpcache[node]);
	// Remove all node indices and whitespace
	cache_id.remove(QRegExp("[a-zA-Z_][a-zA-Z_0-9]*:"));
	cache_id.remove(' ');
	cache_id.remove('\t');
	cache_id.remove('\n');
	return cache_id;
}

#if 0
/*!
	Static function to render CGAL meshes.
	NB! This is just a support function used for development and debugging
*/
CGAL_Nef_polyhedron CGALRenderer::renderCGALMesh(const AbstractPolyNode &node)
{
	// FIXME: Lookup Nef polyhedron in cache.

	// 	print_messages_push();
	
	PolySet *ps = node.render_polyset(AbstractPolyNode::RENDER_CGAL);
	try {
		CGAL_Nef_polyhedron N = ps->renderCSGMesh();
		// FIXME: Insert into cache
		// print_messages_pop();
		node.progress_report();
		
		ps->unlink();
		return N;
	}
	catch (...) { // Don't leak the PolySet on ProgressCancelException
		ps->unlink();
		throw;
	}
}
#endif

#ifdef ENABLE_CGAL

#undef GEN_SURFACE_DEBUG

class CGAL_Build_PolySet : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	typedef CGAL_HDS::Vertex::Point Point;

	const PolySet &ps;
	CGAL_Build_PolySet(const PolySet &ps) : ps(ps) { }

	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);

		QList<PolySet::Point> vertices;
		Grid3d<int> vertices_idx(GRID_FINE);

		for (int i = 0; i < ps.polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps.polygons[i];
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
				if (!vertices_idx.has(p->x, p->y, p->z)) {
					vertices_idx.data(p->x, p->y, p->z) = vertices.size();
					vertices.append(*p);
				}
			}
		}

		B.begin_surface(vertices.size(), ps.polygons.size());
#ifdef GEN_SURFACE_DEBUG
		printf("=== CGAL Surface ===\n");
#endif

		for (int i = 0; i < vertices.size(); i++) {
			const PolySet::Point *p = &vertices[i];
			B.add_vertex(Point(p->x, p->y, p->z));
#ifdef GEN_SURFACE_DEBUG
			printf("%d: %f %f %f\n", i, p->x, p->y, p->z);
#endif
		}

		for (int i = 0; i < ps.polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps.polygons[i];
			QHash<int,int> fc;
			bool facet_is_degenerated = false;
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
				int v = vertices_idx.data(p->x, p->y, p->z);
				if (fc[v]++ > 0)
					facet_is_degenerated = true;
			}
			
			if (!facet_is_degenerated)
				B.begin_facet();
#ifdef GEN_SURFACE_DEBUG
			printf("F:");
#endif
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
#ifdef GEN_SURFACE_DEBUG
				printf(" %d (%f,%f,%f)", vertices_idx.data(p->x, p->y, p->z), p->x, p->y, p->z);
#endif
				if (!facet_is_degenerated)
					B.add_vertex_to_facet(vertices_idx.data(p->x, p->y, p->z));
			}
#ifdef GEN_SURFACE_DEBUG
			if (facet_is_degenerated)
				printf(" (degenerated)");
			printf("\n");
#endif
			if (!facet_is_degenerated)
				B.end_facet();
		}

#ifdef GEN_SURFACE_DEBUG
		printf("====================\n");
#endif
		B.end_surface();

		#undef PointKey
	}
};

#endif /* ENABLE_CGAL */

CGAL_Nef_polyhedron CGALRenderer::renderCGALMesh(const PolySet &ps)
{
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
				for (int i = 0; i < ps.polygons.size(); i++) {
					for (int j = 0; j < ps.polygons[i].size(); j++) {
						double x = ps.polygons[i][j].x;
						double y = ps.polygons[i][j].y;
						if (this->grid.has(x, y)) {
							this->polygons[this->poly_n].append(this->grid.data(x, y));
						} else {
							this->grid.align(x, y) = point_n;
							this->polygons[this->poly_n].append(point_n);
							this->points[point_n] = CGAL_Nef_polyhedron2::Point(x, y);
							point_n++;
						}
					}
					add_edges(this->poly_n);
					this->poly_n++;
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

			CGAL_Nef_polyhedron2 toNef()
			{
				CGAL_Nef_polyhedron2 N;

				QHashIterator< int, QList<int> > it(polygons);
				while (it.hasNext()) {
					it.next();
					std::list<CGAL_Nef_polyhedron2::Point> plist;
					for (int j = 0; j < it.value().size(); j++) {
						int p = it.value()[j];
						plist.push_back(points[p]);
					}
					N += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
				}

				return N;
			}
		};

		PolyReducer pr(ps);
		// printf("Number of polygons before reduction: %d\n", pr.polygons.size());
		pr.reduce();
		// printf("Number of polygons after reduction: %d\n", pr.polygons.size());
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
	else
	{
		CGAL_Polyhedron P;
		CGAL_Build_PolySet builder(ps);
		P.delegate(builder);
#if 0
		std::cout << P;
#endif
		CGAL_Nef_polyhedron3 N(P);
		return CGAL_Nef_polyhedron(N);
	}
	return CGAL_Nef_polyhedron();
}
