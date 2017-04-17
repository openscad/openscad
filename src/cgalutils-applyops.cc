// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "Polygon2d.h"
#include "polyset-utils.h"
#include "grid.h"
#include "node.h"

#include "cgal.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>

#include <CGAL/config.h> 
#include <CGAL/version.h> 

// Apply CGAL bugfix for CGAL-4.5.x
#if CGAL_VERSION_NR > CGAL_VERSION_NUMBER(4,5,1) || CGAL_VERSION_NR < CGAL_VERSION_NUMBER(4,5,0) 
#include <CGAL/convex_hull_3.h>
#else
#include "convex_hull_3_bugfix.h"
#endif

#include "svg.h"
#include "Reindexer.h"
#include "GeometryUtils.h"

#include <map>
#include <queue>
#include <unordered_set>

namespace CGALUtils {

	template<typename Polyhedron>
	bool is_weakly_convex(Polyhedron const& p) {
		for (typename Polyhedron::Edge_const_iterator i = p.edges_begin(); i != p.edges_end(); ++i) {
			typename Polyhedron::Plane_3 p(i->opposite()->vertex()->point(), i->vertex()->point(), i->next()->vertex()->point());
			if (p.has_on_positive_side(i->opposite()->next()->vertex()->point()) &&
				CGAL::squared_distance(p, i->opposite()->next()->vertex()->point()) > 1e-8) {
				return false;
			}
		}
		// Also make sure that there is only one shell:
		std::unordered_set<typename Polyhedron::Facet_const_handle, typename CGAL::Handle_hash_function> visited;
		// c++11
		// visited.reserve(p.size_of_facets());

		std::queue<typename Polyhedron::Facet_const_handle> to_explore;
		to_explore.push(p.facets_begin()); // One arbitrary facet
		visited.insert(to_explore.front());

		while (!to_explore.empty()) {
			typename Polyhedron::Facet_const_handle f = to_explore.front();
			to_explore.pop();
			typename Polyhedron::Facet::Halfedge_around_facet_const_circulator he, end;
			end = he = f->facet_begin();
			CGAL_For_all(he,end) {
				typename Polyhedron::Facet_const_handle o = he->opposite()->facet();

				if (!visited.count(o)) {
					visited.insert(o);
					to_explore.push(o);
				}
			}
		}

		return visited.size() == p.size_of_facets();
	}

/*!
	Applies op to all children and returns the result.
	The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
*/
	CGAL_Nef_polyhedron *applyOperator(const Geometry::Geometries &children, OpenSCADOperator op)
	{
		CGAL_Nef_polyhedron *N = nullptr;
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			// Speeds up n-ary union operations significantly
			CGAL::Nef_nary_union_3<CGAL_Nef_polyhedron3> nary_union;
			int nary_union_num_inserted = 0;
			
			for(const auto &item : children) {
				const shared_ptr<const Geometry> &chgeom = item.second;
				shared_ptr<const CGAL_Nef_polyhedron> chN = 
					dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
				if (!chN) {
					const PolySet *chps = dynamic_cast<const PolySet*>(chgeom.get());
					if (chps) chN.reset(createNefPolyhedronFromGeometry(*chps));
				}
				
				if (op == OpenSCADOperator::UNION) {
					if (!chN->isEmpty()) {
						// nary_union.add_polyhedron() can issue assertion errors:
						// https://github.com/openscad/openscad/issues/802
						nary_union.add_polyhedron(*chN->p3);
						nary_union_num_inserted++;
					}
					continue;
				}
				// Initialize N with first expected geometric object
				if (!N) {
					N = new CGAL_Nef_polyhedron(*chN);
					continue;
				}
				
				// Intersecting something with nothing results in nothing
				if (chN->isEmpty()) {
					if (op == OpenSCADOperator::INTERSECTION) *N = *chN;
					continue;
				}
				
				// empty op <something> => empty
				if (N->isEmpty()) continue;
				
				switch (op) {
				case OpenSCADOperator::INTERSECTION:
					*N *= *chN;
					break;
				case OpenSCADOperator::DIFFERENCE:
					*N -= *chN;
					break;
				case OpenSCADOperator::MINKOWSKI:
					N->minkowski(*chN);
					break;
				default:
					PRINTB("ERROR: Unsupported CGAL operator: %d", static_cast<int>(op));
				}
				item.first->progress_report();
			}

			if (op == OpenSCADOperator::UNION && nary_union_num_inserted > 0) {
				N = new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(nary_union.get_union()));
			}
		}
	// union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
		catch (const CGAL::Failure_exception &e) {
			std::string opstr = op == OpenSCADOperator::INTERSECTION ? "intersection" : op == OpenSCADOperator::DIFFERENCE ? "difference" : op == OpenSCADOperator::UNION ? "union" : "UNKNOWN";
			PRINTB("ERROR: CGAL error in CGALUtils::applyBinaryOperator %s: %s", opstr % e.what());
		}
		CGAL::set_error_behaviour(old_behaviour);
		return N;
	}



	bool applyHull(const Geometry::Geometries &children, PolySet &result)
	{
		typedef CGAL::Epick K;
		// Collect point cloud
		// NB! CGAL's convex_hull_3() doesn't like std::set iterators, so we use a list
		// instead.
		std::list<K::Point_3> points;

		for(const auto &item : children) {
			const shared_ptr<const Geometry> &chgeom = item.second;
			const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(chgeom.get());
			if (N) {
				if (!N->isEmpty()) {
					for (CGAL_Nef_polyhedron3::Vertex_const_iterator i = N->p3->vertices_begin(); i != N->p3->vertices_end(); ++i) {
						points.push_back(vector_convert<K::Point_3>(i->point()));
					}
				}
			} else {
				const PolySet *ps = dynamic_cast<const PolySet *>(chgeom.get());
				if (ps) {
					for(const auto &p : ps->polygons) {
						for(const auto &v : p) {
							points.push_back(K::Point_3(v[0], v[1], v[2]));
						}
					}
				}
			}
		}

		if (points.size() <= 3) return false;

		// Apply hull
		bool success = false;
		if (points.size() >= 4) {
			CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
			try {
				CGAL::Polyhedron_3<K> r;
				CGAL::convex_hull_3(points.begin(), points.end(), r);
                            PRINTDB("After hull vertices: %d", r.size_of_vertices());
                            PRINTDB("After hull facets: %d", r.size_of_facets());
                            PRINTDB("After hull closed: %d", r.is_closed());
                            PRINTDB("After hull valid: %d", r.is_valid());
				success = !createPolySetFromPolyhedron(r, result);
			}
			catch (const CGAL::Failure_exception &e) {
				PRINTB("ERROR: CGAL error in applyHull(): %s", e.what());
			}
			CGAL::set_error_behaviour(old_behaviour);
		}
		return success;
	}


	/*!
		children cannot contain nullptr objects
	*/
	Geometry const * applyMinkowski(const Geometry::Geometries &children)
	{
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		CGAL::Timer t,t_tot;
		assert(children.size() >= 2);
		Geometry::Geometries::const_iterator it = children.begin();
		t_tot.start();
		Geometry const* operands[2] = {it->second.get(), nullptr};
		try {
			while (++it != children.end()) {
				operands[1] = it->second.get();

				typedef CGAL::Epick Hull_kernel;

				std::list<CGAL_Polyhedron> P[2];
				std::list<CGAL::Polyhedron_3<Hull_kernel>> result_parts;

				for (size_t i = 0; i < 2; i++) {
					CGAL_Polyhedron poly;

					const PolySet * ps = dynamic_cast<const PolySet *>(operands[i]);

					const CGAL_Nef_polyhedron * nef = dynamic_cast<const CGAL_Nef_polyhedron *>(operands[i]);

					if (ps) CGALUtils::createPolyhedronFromPolySet(*ps, poly);
					else if (nef && nef->p3->is_simple()) nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>(*nef->p3, poly);
					else throw 0;

					if ((ps && ps->is_convex()) ||
							(!ps && is_weakly_convex(poly))) {
						PRINTDB("Minkowski: child %d is convex and %s",i % (ps?"PolySet":"Nef"));
						P[i].push_back(poly);
					} else {
						CGAL_Nef_polyhedron3 decomposed_nef;

						if (ps) {
							PRINTDB("Minkowski: child %d is nonconvex PolySet, transforming to Nef and decomposing...", i);
							CGAL_Nef_polyhedron *p = createNefPolyhedronFromGeometry(*ps);
							if (!p->isEmpty()) decomposed_nef = *p->p3;
							delete p;
						} else {
							PRINTDB("Minkowski: child %d is nonconvex Nef, decomposing...",i);
							decomposed_nef = *nef->p3;
						}

						t.start();
						CGAL::convex_decomposition_3(decomposed_nef);

						// the first volume is the outer volume, which ignored in the decomposition
						CGAL_Nef_polyhedron3::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
						for(; ci != decomposed_nef.volumes_end(); ++ci) {
							if(ci->mark()) {
								CGAL_Polyhedron poly;
								decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
								P[i].push_back(poly);
							}
						}


						PRINTDB("Minkowski: decomposed into %d convex parts", P[i].size());
						t.stop();
						PRINTDB("Minkowski: decomposition took %f s", t.time());
					}
				}

				std::vector<Hull_kernel::Point_3> points[2];
				std::vector<Hull_kernel::Point_3> minkowski_points;

				for (size_t i = 0; i < P[0].size(); i++) {
					for (size_t j = 0; j < P[1].size(); j++) {
						t.start();
						points[0].clear();
						points[1].clear();

						for (int k = 0; k < 2; k++) {
							std::list<CGAL_Polyhedron>::iterator it = P[k].begin();
							std::advance(it, k==0?i:j);

							CGAL_Polyhedron const& poly = *it;
							points[k].reserve(poly.size_of_vertices());

							for (CGAL_Polyhedron::Vertex_const_iterator pi = poly.vertices_begin(); pi != poly.vertices_end(); ++pi) {
								CGAL_Polyhedron::Point_3 const& p = pi->point();
								points[k].push_back(Hull_kernel::Point_3(to_double(p[0]),to_double(p[1]),to_double(p[2])));
							}
						}

						minkowski_points.clear();
						minkowski_points.reserve(points[0].size() * points[1].size());
						for (size_t i = 0; i < points[0].size(); i++) {
							for (size_t j = 0; j < points[1].size(); j++) {
								minkowski_points.push_back(points[0][i]+(points[1][j]-CGAL::ORIGIN));
							}
						}

						if (minkowski_points.size() <= 3) {
							t.stop();
							continue;
						}


						CGAL::Polyhedron_3<Hull_kernel> result;
						t.stop();
						PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms", points[0].size() % points[1].size() % minkowski_points.size() % (t.time()*1000));
						t.reset();

						t.start();

						CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), result);

						std::vector<Hull_kernel::Point_3> strict_points;
						strict_points.reserve(minkowski_points.size());

						for (CGAL::Polyhedron_3<Hull_kernel>::Vertex_iterator i = result.vertices_begin(); i != result.vertices_end(); ++i) {
							Hull_kernel::Point_3 const& p = i->point();

							CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle h,e;
							h = i->halfedge();
							e = h;
							bool collinear = false;
							bool coplanar = true;

							do {
								Hull_kernel::Point_3 const& q = h->opposite()->vertex()->point();
								if (coplanar && !CGAL::coplanar(p,q,
																h->next_on_vertex()->opposite()->vertex()->point(),
																h->next_on_vertex()->next_on_vertex()->opposite()->vertex()->point())) {
									coplanar = false;
								}


								for (CGAL::Polyhedron_3<Hull_kernel>::Vertex::Halfedge_handle j = h->next_on_vertex();
									 j != h && !collinear && ! coplanar;
									 j = j->next_on_vertex()) {

									Hull_kernel::Point_3 const& r = j->opposite()->vertex()->point();
									if (CGAL::collinear(p,q,r)) {
										collinear = true;
									}
								}

								h = h->next_on_vertex();
							} while (h != e && !collinear);

							if (!collinear && !coplanar)
								strict_points.push_back(p);
						}

						result.clear();
						CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), result);


						t.stop();
						PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
						t.reset();

						result_parts.push_back(result);
					}
				}

				if (it != boost::next(children.begin()))
					delete operands[0];

				if (result_parts.size() == 1) {
					PolySet *ps = new PolySet(3,true);
					createPolySetFromPolyhedron(*result_parts.begin(), *ps);
					operands[0] = ps;
				} else if (!result_parts.empty()) {
					t.start();
					PRINTDB("Minkowski: Computing union of %d parts",result_parts.size());
					Geometry::Geometries fake_children;
					for (std::list<CGAL::Polyhedron_3<Hull_kernel>>::iterator i = result_parts.begin(); i != result_parts.end(); ++i) {
						PolySet ps(3,true);
						createPolySetFromPolyhedron(*i, ps);
						fake_children.push_back(std::make_pair((const AbstractNode*)nullptr,
															   shared_ptr<const Geometry>(createNefPolyhedronFromGeometry(ps))));
					}
					CGAL_Nef_polyhedron *N = CGALUtils::applyOperator(fake_children, OpenSCADOperator::UNION);
					// FIXME: This hould really never throw.
					// Assert once we figured out what went wrong with issue #1069?
					if (!N) throw 0;
					t.stop();
					PRINTDB("Minkowski: Union done: %f s",t.time());
					t.reset();
					operands[0] = N;
				} else {
                    operands[0] = new CGAL_Nef_polyhedron();
				}
			}

			t_tot.stop();
			PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
			t_tot.reset();
			CGAL::set_error_behaviour(old_behaviour);
			return operands[0];
		}
		catch (...) {
			// If anything throws we simply fall back to Nef Minkowski
			PRINTD("Minkowski: Falling back to Nef Minkowski");

			CGAL_Nef_polyhedron *N = applyOperator(children, OpenSCADOperator::MINKOWSKI);
			CGAL::set_error_behaviour(old_behaviour);
			return N;
		}
	}
}; // namespace CGALUtils


#endif // ENABLE_CGAL








