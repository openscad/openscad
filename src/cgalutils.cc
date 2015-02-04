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
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>

namespace /* anonymous */ {
	template<typename Result, typename V>
	Result vector_convert(V const& v) {
		return Result(CGAL::to_double(v[0]),CGAL::to_double(v[1]),CGAL::to_double(v[2]));
	}
}

static CGAL_Nef_polyhedron *createNefPolyhedronFromPolySet(const PolySet &ps)
{
	if (ps.isEmpty()) return new CGAL_Nef_polyhedron();
	assert(ps.getDimension() == 3);

	// Since is_convex doesn't work well with non-planar faces,
	// we tessellate the polyset before checking.
	PolySet psq(ps);
	psq.quantizeVertices();
	PolySet ps_tri(3, psq.convexValue());
	PolysetUtils::tessellate_faces(psq, ps_tri);
	if (ps_tri.is_convex()) {
		typedef CGAL::Epick K;
		// Collect point cloud
		// FIXME: Use unordered container (need hash)
		// NB! CGAL's convex_hull_3() doesn't like std::set iterators, so we use a list
		// instead.
		std::list<K::Point_3> points;
		BOOST_FOREACH(const Polygon &poly, ps.polygons) {
			BOOST_FOREACH(const Vector3d &p, poly) {
				points.push_back(vector_convert<K::Point_3>(p));
			}
		}

		if (points.size() <= 3) return new CGAL_Nef_polyhedron();;

		// Apply hull
		CGAL::Polyhedron_3<K> r;
		CGAL::convex_hull_3(points.begin(), points.end(), r);
		CGAL_Polyhedron r_exact;
		CGALUtils::copyPolyhedron(r, r_exact);
		return new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(r_exact));
	}

	CGAL_Nef_polyhedron3 *N = NULL;
	bool plane_error = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		bool err = CGALUtils::createPolyhedronFromPolySet(psq, P);
		 if (!err) {
		 	PRINTDB("Polyhedron is closed: %d", P.is_closed());
		 	PRINTDB("Polyhedron is valid: %d", P.is_valid(false, 0));
		 }

		if (!err) N = new CGAL_Nef_polyhedron3(P);
	}
	catch (const CGAL::Assertion_exception &e) {
		if (std::string(e.what()).find("Plane_constructor")!=std::string::npos &&
            std::string(e.what()).find("has_on")!=std::string::npos) {
				PRINT("PolySet has nonplanar faces. Attempting alternate construction");
				plane_error=true;
		} else {
			PRINTB("ERROR: CGAL error in CGAL_Nef_polyhedron3(): %s", e.what());
		}
	}
	if (plane_error) try {
			CGAL_Polyhedron P;
			bool err = CGALUtils::createPolyhedronFromPolySet(ps_tri, P);
            if (!err) {
                PRINTDB("Polyhedron is closed: %d", P.is_closed());
                PRINTDB("Polyhedron is valid: %d", P.is_valid(false, 0));
            }
			if (!err) N = new CGAL_Nef_polyhedron3(P);
		}
		catch (const CGAL::Assertion_exception &e) {
			PRINTB("ERROR: Alternate construction failed. CGAL error in CGAL_Nef_polyhedron3(): %s", e.what());
		}
	CGAL::set_error_behaviour(old_behaviour);
	return new CGAL_Nef_polyhedron(N);
}

static CGAL_Nef_polyhedron *createNefPolyhedronFromPolygon2d(const Polygon2d &polygon)
{
	shared_ptr<PolySet> ps(polygon.tessellate());
	return createNefPolyhedronFromPolySet(*ps);
}

namespace CGALUtils {

	bool applyHull(const Geometry::ChildList &children, PolySet &result)
	{
		typedef CGAL::Epick K;
		// Collect point cloud
		// NB! CGAL's convex_hull_3() doesn't like std::set iterators, so we use a list
		// instead.
		std::list<K::Point_3> points;

		BOOST_FOREACH(const Geometry::ChildItem &item, children) {
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
					BOOST_FOREACH(const Polygon &p, ps->polygons) {
						BOOST_FOREACH(const Vector3d &v, p) {
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
			catch (const CGAL::Assertion_exception &e) {
				PRINTB("ERROR: CGAL error in applyHull(): %s", e.what());
			}
			CGAL::set_error_behaviour(old_behaviour);
		}
		return success;
	}

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
		boost::unordered_set<typename Polyhedron::Facet_const_handle, typename CGAL::Handle_hash_function> visited;
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
		children cannot contain NULL objects
	*/
	Geometry const * applyMinkowski(const Geometry::ChildList &children)
	{
		CGAL::Timer t,t_tot;
		assert(children.size() >= 2);
		Geometry::ChildList::const_iterator it = children.begin();
		t_tot.start();
		Geometry const* operands[2] = {it->second.get(), NULL};
		try {
			while (++it != children.end()) {
				operands[1] = it->second.get();

				typedef CGAL::Epick Hull_kernel;

				std::list<CGAL_Polyhedron> P[2];
				std::list<CGAL::Polyhedron_3<Hull_kernel> > result_parts;

				for (int i = 0; i < 2; i++) {
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
						for (int i = 0; i < points[0].size(); i++) {
							for (int j = 0; j < points[1].size(); j++) {
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
					Geometry::ChildList fake_children;
					for (std::list<CGAL::Polyhedron_3<Hull_kernel> >::iterator i = result_parts.begin(); i != result_parts.end(); ++i) {
						PolySet ps(3,true);
						createPolySetFromPolyhedron(*i, ps);
						fake_children.push_back(std::make_pair((const AbstractNode*)NULL,
															   shared_ptr<const Geometry>(createNefPolyhedronFromGeometry(ps))));
					}
					CGAL_Nef_polyhedron *N = CGALUtils::applyOperator(fake_children, OPENSCAD_UNION);
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
			return operands[0];
		}
		catch (...) {
			// If anything throws we simply fall back to Nef Minkowski
			PRINTD("Minkowski: Falling back to Nef Minkowski");

			CGAL_Nef_polyhedron *N = applyOperator(children, OPENSCAD_MINKOWSKI);
			return N;
		}
	}
	
/*!
	Applies op to all children and returns the result.
	The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
*/
	CGAL_Nef_polyhedron *applyOperator(const Geometry::ChildList &children, OpenSCADOperator op)
	{
		CGAL_Nef_polyhedron *N = NULL;
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			// Speeds up n-ary union operations significantly
			CGAL::Nef_nary_union_3<CGAL_Nef_polyhedron3> nary_union;
			int nary_union_num_inserted = 0;
			
			BOOST_FOREACH(const Geometry::ChildItem &item, children) {
				const shared_ptr<const Geometry> &chgeom = item.second;
				shared_ptr<const CGAL_Nef_polyhedron> chN = 
					dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
				if (!chN) {
					const PolySet *chps = dynamic_cast<const PolySet*>(chgeom.get());
					if (chps) chN.reset(createNefPolyhedronFromGeometry(*chps));
				}
				
				if (op == OPENSCAD_UNION) {
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
					if (op == OPENSCAD_INTERSECTION) *N = *chN;
					continue;
				}
				
				// empty op <something> => empty
				if (N->isEmpty()) continue;
				
				switch (op) {
				case OPENSCAD_INTERSECTION:
					*N *= *chN;
					break;
				case OPENSCAD_DIFFERENCE:
					*N -= *chN;
					break;
				case OPENSCAD_MINKOWSKI:
					N->minkowski(*chN);
					break;
				default:
					PRINTB("ERROR: Unsupported CGAL operator: %d", op);
				}
				item.first->progress_report();
			}

			if (op == OPENSCAD_UNION && nary_union_num_inserted > 0) {
				N = new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(nary_union.get_union()));
			}
		}
	// union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
		catch (const CGAL::Failure_exception &e) {
			std::string opstr = op == OPENSCAD_INTERSECTION ? "intersection" : op == OPENSCAD_DIFFERENCE ? "difference" : op == OPENSCAD_UNION ? "union" : "UNKNOWN";
			PRINTB("ERROR: CGAL error in CGALUtils::applyBinaryOperator %s: %s", opstr % e.what());
		}
		CGAL::set_error_behaviour(old_behaviour);
		return N;
	}

/*!
	Modifies target by applying op to target and src:
	target = target [op] src
*/
//FIXME: Old, can be removed:
#if 0
	void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op)
	{
		if (target.getDimension() != 2 && target.getDimension() != 3) {
			assert(false && "Dimension of Nef polyhedron must be 2 or 3");
		}
		if (src.isEmpty()) {
			// Intersecting something with nothing results in nothing
			if (op == OPENSCAD_INTERSECTION) target = src;
			// else keep target unmodified
			return;
		}
		if (src.isEmpty()) return; // Empty polyhedron. This can happen for e.g. square([0,0])
		if (target.isEmpty() && op != OPENSCAD_UNION) return; // empty op <something> => empty
		if (target.getDimension() != src.getDimension()) return; // If someone tries to e.g. union 2d and 3d objects

		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			switch (op) {
			case OPENSCAD_UNION:
				if (target.isEmpty()) target = *new CGAL_Nef_polyhedron(src);
				else target += src;
				break;
			case OPENSCAD_INTERSECTION:
				target *= src;
				break;
			case OPENSCAD_DIFFERENCE:
				target -= src;
				break;
			case OPENSCAD_MINKOWSKI:
				target.minkowski(src);
				break;
			default:
				PRINTB("ERROR: Unsupported CGAL operator: %d", op);
			}
		}
		catch (const CGAL::Failure_exception &e) {
			// union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
			std::string opstr = op == OPENSCAD_UNION ? "union" : op == OPENSCAD_INTERSECTION ? "intersection" : op == OPENSCAD_DIFFERENCE ? "difference" : op == OPENSCAD_MINKOWSKI ? "minkowski" : "UNKNOWN";
			PRINTB("ERROR: CGAL error in CGALUtils::applyBinaryOperator %s: %s", opstr % e.what());

			// Errors can result in corrupt polyhedrons, so put back the old one
			target = src;
		}
		CGAL::set_error_behaviour(old_behaviour);
	}
#endif

	static void add_outline_to_poly(CGAL_Nef_polyhedron2::Explorer &explorer,
									CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator circ,
									CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator end,
									bool positive,
									Polygon2d *poly) {
		Outline2d outline;

		CGAL_For_all(circ, end) {
			if (explorer.is_standard(explorer.target(circ))) {
				CGAL_Nef_polyhedron2::Explorer::Point ep = explorer.point(explorer.target(circ));
				outline.vertices.push_back(Vector2d(to_double(ep.x()),
													to_double(ep.y())));
			}
		}

		if (!outline.vertices.empty()) {
			outline.positive = positive;
			poly->addOutline(outline);
		}
	}

	static Polygon2d *convertToPolygon2d(const CGAL_Nef_polyhedron2 &p2)
	{
		Polygon2d *poly = new Polygon2d;
		
		typedef CGAL_Nef_polyhedron2::Explorer Explorer;
		typedef Explorer::Face_const_iterator fci_t;
		typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
		Explorer E = p2.explorer();

		for (fci_t fit = E.faces_begin(), facesend = E.faces_end(); fit != facesend; ++fit)	{
			if (!fit->mark()) continue;

			heafcc_t fcirc(E.face_cycle(fit)), fend(fcirc);

			add_outline_to_poly(E, fcirc, fend, true, poly);

			for (CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j = E.holes_begin(fit);
				 j != E.holes_end(fit); ++j) {
				CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator hcirc(j), hend(hcirc);

				add_outline_to_poly(E, hcirc, hend, false, poly);
			}
		}

		poly->setSanitized(true);
		return poly;
	}

	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut)
	{
		Polygon2d *poly = NULL;
		if (N.getDimension() != 3) return poly;

		CGAL_Nef_polyhedron newN;
		if (cut) {
			CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
			try {
				CGAL_Nef_polyhedron3::Plane_3 xy_plane = CGAL_Nef_polyhedron3::Plane_3(0,0,1,0);
				newN.p3.reset(new CGAL_Nef_polyhedron3(N.p3->intersection(xy_plane, CGAL_Nef_polyhedron3::PLANE_ONLY)));
			}
			catch (const CGAL::Failure_exception &e) {
				PRINTDB("CGALUtils::project during plane intersection: %s", e.what());
				try {
					PRINTD("Trying alternative intersection using very large thin box: ");
					std::vector<CGAL_Point_3> pts;
					// dont use z of 0. there are bugs in CGAL.
					double inf = 1e8;
					double eps = 0.001;
					CGAL_Point_3 minpt(-inf, -inf, -eps);
					CGAL_Point_3 maxpt( inf,  inf,  eps);
					CGAL_Iso_cuboid_3 bigcuboid(minpt, maxpt);
					for (int i=0;i<8;i++) pts.push_back(bigcuboid.vertex(i));
					CGAL_Polyhedron bigbox;
					CGAL::convex_hull_3(pts.begin(), pts.end(), bigbox);
					CGAL_Nef_polyhedron3 nef_bigbox(bigbox);
					newN.p3.reset(new CGAL_Nef_polyhedron3(nef_bigbox.intersection(*N.p3)));
				}
				catch (const CGAL::Failure_exception &e) {
					PRINTB("ERROR: CGAL error in CGALUtils::project during bigbox intersection: %s", e.what());
				}
			}
				
			if (!newN.p3 || newN.p3->is_empty()) {
				CGAL::set_error_behaviour(old_behaviour);
				PRINT("WARNING: projection() failed.");
				return poly;
			}
				
			PRINTDB("%s",OpenSCAD::svg_header(480, 100000));
			try {
				ZRemover zremover;
				CGAL_Nef_polyhedron3::Volume_const_iterator i;
				CGAL_Nef_polyhedron3::Shell_entry_const_iterator j;
				CGAL_Nef_polyhedron3::SFace_const_handle sface_handle;
				for (i = newN.p3->volumes_begin(); i != newN.p3->volumes_end(); ++i) {
					PRINTDB("<!-- volume. mark: %s -->",i->mark());
					for (j = i->shells_begin(); j != i->shells_end(); ++j) {
						PRINTDB("<!-- shell. (vol mark was: %i)", i->mark());;
						sface_handle = CGAL_Nef_polyhedron3::SFace_const_handle(j);
						newN.p3->visit_shell_objects(sface_handle , zremover);
						PRINTD("<!-- shell. end. -->");
					}
					PRINTD("<!-- volume end. -->");
				}
				poly = convertToPolygon2d(*zremover.output_nefpoly2d);
			}	catch (const CGAL::Failure_exception &e) {
				PRINTB("ERROR: CGAL error in CGALUtils::project while flattening: %s", e.what());
			}
			PRINTD("</svg>");
				
			CGAL::set_error_behaviour(old_behaviour);
		}
		// In projection mode all the triangles are projected manually into the XY plane
		else {
			PolySet ps(3);
			bool err = CGALUtils::createPolySetFromNefPolyhedron3(*N.p3, ps);
			if (err) {
				PRINT("ERROR: Nef->PolySet failed");
				return poly;
			}
			poly = PolysetUtils::project(ps);
		}
		return poly;
	}

	CGAL_Iso_cuboid_3 boundingBox(const CGAL_Nef_polyhedron3 &N)
	{
		CGAL_Iso_cuboid_3 result(0,0,0,0,0,0);
		CGAL_Nef_polyhedron3::Vertex_const_iterator vi;
		std::vector<CGAL_Nef_polyhedron3::Point_3> points;
		// can be optimized by rewriting bounding_box to accept vertices
		CGAL_forall_vertices(vi, N)
		points.push_back(vi->point());
		if (points.size()) result = CGAL::bounding_box(points.begin(), points.end());
		return result;
	}

	namespace {

		// lexicographic comparison
		bool operator < (Vector3d const& a, Vector3d const& b) {
			for (int i = 0; i < 3; i++) {
				if (a[i] < b[i]) return true;
				else if (a[i] == b[i]) continue;
				return false;
			}
			return false;
		}
	}

	struct VecPairCompare {
		bool operator ()(std::pair<Vector3d, Vector3d> const& a,
						 std::pair<Vector3d, Vector3d> const& b) const {
			return a.first < b.first || (!(b.first < a.first) && a.second < b.second);
		}
	};


  /*!
		Check if all faces of a polyset is within 0.1 degree of being convex.
		
		NB! This function can give false positives if the polyset contains
		non-planar faces. To be on the safe side, consider passing a tessellated polyset.
		See issue #1061.
	*/
	bool is_approximately_convex(const PolySet &ps) {

		const double angle_threshold = cos(.1/180*M_PI); // .1°

		typedef CGAL::Simple_cartesian<double> K;
		typedef K::Vector_3 Vector;
		typedef K::Point_3 Point;
		typedef K::Plane_3 Plane;

		// compute edge to face relations and plane equations
		typedef std::pair<Vector3d,Vector3d> Edge;
		typedef std::map<Edge, int, VecPairCompare> Edge_to_facet_map;
		Edge_to_facet_map edge_to_facet_map;
		std::vector<Plane> facet_planes; facet_planes.reserve(ps.polygons.size());

		for (int i = 0; i < ps.polygons.size(); i++) {
			Plane plane;
			size_t N = ps.polygons[i].size();
			if (N >= 3) {
				std::vector<Point> v(N);
				for (int j = 0; j < N; j++) {
					v[j] = vector_convert<Point>(ps.polygons[i][j]);
					Edge edge(ps.polygons[i][j],ps.polygons[i][(j+1)%N]);
					if (edge_to_facet_map.count(edge)) return false; // edge already exists: nonmanifold
					edge_to_facet_map[edge] = i;
				}
				Vector normal;
				CGAL::normal_vector_newell_3(v.begin(), v.end(), normal);
				plane = Plane(v[0], normal);
			}
			facet_planes.push_back(plane);
		}

		for (int i = 0; i < ps.polygons.size(); i++) {
			size_t N = ps.polygons[i].size();
			if (N < 3) continue;
			for (int j = 0; j < N; j++) {
				Edge other_edge(ps.polygons[i][(j+1)%N], ps.polygons[i][j]);
				if (edge_to_facet_map.count(other_edge) == 0) return false;//
				//Edge_to_facet_map::const_iterator it = edge_to_facet_map.find(other_edge);
				//if (it == edge_to_facet_map.end()) return false; // not a closed manifold
				//int other_facet = it->second;
				int other_facet = edge_to_facet_map[other_edge];

				Point p = vector_convert<Point>(ps.polygons[i][(j+2)%N]);

				if (facet_planes[other_facet].has_on_positive_side(p)) {
					// Check angle
					Vector u = facet_planes[other_facet].orthogonal_vector();
					Vector v = facet_planes[i].orthogonal_vector();

					double cos_angle = u / sqrt(u*u) * v / sqrt(v*v);
					if (cos_angle < angle_threshold) {
						return false;
					}
				}
			}
		}

		std::set<int> explored_facets;
		std::queue<int> facets_to_visit;
		facets_to_visit.push(0);
		explored_facets.insert(0);

		while(!facets_to_visit.empty()) {
			int f = facets_to_visit.front(); facets_to_visit.pop();

			for (int i = 0; i < ps.polygons[f].size(); i++) {
				int j = (i+1) % ps.polygons[f].size();
				Edge_to_facet_map::iterator it = edge_to_facet_map.find(Edge(ps.polygons[f][j], ps.polygons[f][i]));
				if (it == edge_to_facet_map.end()) return false; // Nonmanifold
				if (!explored_facets.count(it->second)) {
					explored_facets.insert(it->second);
					facets_to_visit.push(it->second);
				}
			}
		}

		// Make sure that we were able to reach all polygons during our visit
		return explored_facets.size() == ps.polygons.size();
	}


/*
	Create a PolySet from a Nef Polyhedron 3. return false on success, 
	true on failure. The trick to this is that Nef Polyhedron3 faces have 
	'holes' in them. . . while PolySet (and many other 3d polyhedron 
	formats) do not allow for holes in their faces. The function documents 
	the method used to deal with this
*/
#if 0
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		bool err = false;
		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			std::vector<CGAL_Polygon_3> polyholes;
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (hfaceti->incident_volume()->mark()) continue;
			CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
			CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
				CGAL_Polygon_3 polygon;
				CGAL_For_all(c1, c2) {
					CGAL_Point_3 p = c1->source()->center_vertex()->point();
					polygon.push_back(p);
				}
				polyholes.push_back(polygon);
			}

			/* at this stage, we have a sequence of polygons. the first
				 is the "outside edge' or 'body' or 'border', and the rest of the
				 polygons are 'holes' within the first. there are several
				 options here to get rid of the holes. we choose to go ahead
				 and let the tessellater deal with the holes, and then
				 just output the resulting 3d triangles*/

			CGAL::Vector_3<CGAL_Kernel3> nvec = plane.orthogonal_vector();
			K::Vector_3 normal(CGAL::to_double(nvec.x()), CGAL::to_double(nvec.y()), CGAL::to_double(nvec.z()));
			std::vector<CGAL_Polygon_3> triangles;
			bool err = CGALUtils::tessellate3DFaceWithHoles(polyholes, triangles, plane);
			if (!err) {
				BOOST_FOREACH(const CGAL_Polygon_3 &p, triangles) {
					if (p.size() != 3) {
						PRINT("WARNING: triangle doesn't have 3 points. skipping");
						continue;
					}
					ps.append_poly();
					ps.append_vertex(CGAL::to_double(p[2].x()), CGAL::to_double(p[2].y()), CGAL::to_double(p[2].z()));
					ps.append_vertex(CGAL::to_double(p[1].x()), CGAL::to_double(p[1].y()), CGAL::to_double(p[1].z()));
					ps.append_vertex(CGAL::to_double(p[0].x()), CGAL::to_double(p[0].y()), CGAL::to_double(p[0].z()));
				}
			}
		}
		return err;
	}
#endif
#if 0
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		bool err = false;
		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			std::vector<CGAL_Polygon_3> polyholes;
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (hfaceti->incident_volume()->mark()) continue;
			CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
			CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
				CGAL_Polygon_3 polygon;
				CGAL_For_all(c1, c2) {
					CGAL_Point_3 p = c1->source()->center_vertex()->point();
					polygon.push_back(p);
				}
				polyholes.push_back(polygon);
			}

			/* at this stage, we have a sequence of polygons. the first
				 is the "outside edge' or 'body' or 'border', and the rest of the
				 polygons are 'holes' within the first. there are several
				 options here to get rid of the holes. we choose to go ahead
				 and let the tessellater deal with the holes, and then
				 just output the resulting 3d triangles*/

			CGAL::Vector_3<CGAL_Kernel3> nvec = plane.orthogonal_vector();
			K::Vector_3 normal(CGAL::to_double(nvec.x()), CGAL::to_double(nvec.y()), CGAL::to_double(nvec.z()));
			std::vector<Polygon> triangles;
			bool err = CGALUtils::tessellate3DFaceWithHolesNew(polyholes, triangles, plane);
			if (!err) {
				BOOST_FOREACH(const Polygon &p, triangles) {
					if (p.size() != 3) {
						PRINT("WARNING: triangle doesn't have 3 points. skipping");
						continue;
					}
					ps.append_poly();
					ps.append_vertex(p[0].x(), p[0].y(), p[0].z());
					ps.append_vertex(p[1].x(), p[1].y(), p[1].z());
					ps.append_vertex(p[2].x(), p[2].y(), p[2].z());
				}
			}
		}
		return err;
	}
#endif
#if 0
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		bool err = false;
		// Grid all vertices in a Nef polyhedron to merge close vertices.
		Grid3d<int> grid(GRID_FINE);
		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			PolyholeK polyholes;
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (hfaceti->incident_volume()->mark()) continue;
			CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
			CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
				PolygonK polygon;
				std::vector<int> indices; // Vertex indices in one polygon
				CGAL_For_all(c1, c2) {
					CGAL_Point_3 p = c1->source()->center_vertex()->point();
					Vector3d v = vector_convert<Vector3d>(p);
					indices.push_back(grid.align(v));
					polygon.push_back(Vertex3K(v[0], v[1], v[2]));
				}
				// Remove consecutive duplicate vertices
				PolygonK::iterator currp = polygon.begin();
				for (int i=0;i<indices.size();i++) {
					if (indices[i] != indices[(i+1)%indices.size()]) {
						(*currp++) = polygon[i];
					}
				}
				polygon.erase(currp, polygon.end());
				if (polygon.size() >= 3) polyholes.push_back(polygon);
			}

			/* at this stage, we have a sequence of polygons. the first
				 is the "outside edge' or 'body' or 'border', and the rest of the
				 polygons are 'holes' within the first. there are several
				 options here to get rid of the holes. we choose to go ahead
				 and let the tessellater deal with the holes, and then
				 just output the resulting 3d triangles*/

			// We cannot trust the plane from Nef polyhedron to be correct.
			// Passing an incorrect normal vector can cause a crash in the constrained delaunay triangulator
      // See http://cgal-discuss.949826.n4.nabble.com/Nef3-Wrong-normal-vector-reported-causes-triangulator-crash-tt4660282.html
			// CGAL::Vector_3<CGAL_Kernel3> nvec = plane.orthogonal_vector();
			// K::Vector_3 normal(CGAL::to_double(nvec.x()), CGAL::to_double(nvec.y()), CGAL::to_double(nvec.z()));
			std::vector<Polygon> triangles;
			bool err = CGALUtils::tessellatePolygonWithHolesNew(polyholes, triangles, NULL);
			if (!err) {
				BOOST_FOREACH(const Polygon &p, triangles) {
					if (p.size() != 3) {
						PRINT("WARNING: triangle doesn't have 3 points. skipping");
						continue;
					}
					ps.append_poly();
					ps.append_vertex(p[0].x(), p[0].y(), p[0].z());
					ps.append_vertex(p[1].x(), p[1].y(), p[1].z());
					ps.append_vertex(p[2].x(), p[2].y(), p[2].z());
				}
			}
		}
		return err;
	}
#endif
#if 0
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		bool err = false;
		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			PolyholeK polyholes;
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (hfaceti->incident_volume()->mark()) continue;
			CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
			CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
				PolygonK polygon;
				CGAL_For_all(c1, c2) {
					CGAL_Point_3 p = c1->source()->center_vertex()->point();
					float v[3] = { CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z()) };
					polygon.push_back(Vertex3K(v[0], v[1], v[2]));
				}
				polyholes.push_back(polygon);
			}

			std::cout << "---\n";
			BOOST_FOREACH(const PolygonK &poly, polyholes) {
				BOOST_FOREACH(const Vertex3K &v, poly) {
					std::cout << v.x() << "," << v.y() << "," << v.z() << "\n";
				}
				std::cout << "\n";
			}
			std::cout << "-\n";

			/* at this stage, we have a sequence of polygons. the first
				 is the "outside edge' or 'body' or 'border', and the rest of the
				 polygons are 'holes' within the first. there are several
				 options here to get rid of the holes. we choose to go ahead
				 and let the tessellater deal with the holes, and then
				 just output the resulting 3d triangles*/

			// We cannot trust the plane from Nef polyhedron to be correct.
			// Passing an incorrect normal vector can cause a crash in the constrained delaunay triangulator
      // See http://cgal-discuss.949826.n4.nabble.com/Nef3-Wrong-normal-vector-reported-causes-triangulator-crash-tt4660282.html
			// CGAL::Vector_3<CGAL_Kernel3> nvec = plane.orthogonal_vector();
			// K::Vector_3 normal(CGAL::to_double(nvec.x()), CGAL::to_double(nvec.y()), CGAL::to_double(nvec.z()));
			std::vector<Polygon> triangles;
			bool err = CGALUtils::tessellatePolygonWithHolesNew(polyholes, triangles, NULL);
			if (!err) {
				BOOST_FOREACH(const Polygon &p, triangles) {
					if (p.size() != 3) {
						PRINT("WARNING: triangle doesn't have 3 points. skipping");
						continue;
					}
					ps.append_poly();
					ps.append_vertex(p[0].x(), p[0].y(), p[0].z());
					ps.append_vertex(p[1].x(), p[1].y(), p[1].z());
					ps.append_vertex(p[2].x(), p[2].y(), p[2].z());
					// std::cout << p[0].x() << "," << p[0].y() << "," << p[0].z() << "\n";
					// std::cout << p[1].x() << "," << p[1].y() << "," << p[1].z() << "\n";
					// std::cout << p[2].x() << "," << p[2].y() << "," << p[2].z() << "\n\n";
				}
			}
		}
		return err;
	}
#endif
#if 1
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		bool err = false;
		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			// Since we're downscaling to float, vertices might merge during this conversion.
			// To avoid passing equal vertices to the tessellator, we remove consecutively identical
			// vertices.
			Reindexer<Vector3f> uniqueVertices;
			IndexedPolygons polyhole;
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (hfaceti->incident_volume()->mark()) continue;
			CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
			CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
				polyhole.faces.push_back(IndexedFace());
				IndexedFace &currface = polyhole.faces.back();
				CGAL_For_all(c1, c2) {
					CGAL_Point_3 p = c1->source()->center_vertex()->point();
					// Create vertex indices and remove consecutive duplicate vertices
					int idx = uniqueVertices.lookup(vector_convert<Vector3f>(p));
					if (currface.empty() || idx != currface.back()) currface.push_back(idx);
				}
				if (currface.front() == currface.back()) currface.pop_back();
				if (currface.size() < 3) polyhole.faces.pop_back(); // Cull empty triangles
			}
			uniqueVertices.copy(std::back_inserter(polyhole.vertices));

#if 0 // For debugging
			std::cerr << "---\n";
			std::cerr.precision(20);
			BOOST_FOREACH(const IndexedFace &poly, polyhole.faces) {
				BOOST_FOREACH(int i, poly) {
					std::cerr << polyhole.vertices[i][0] << "," << polyhole.vertices[i][1] << "," << polyhole.vertices[i][2] << "\n";
				}
				std::cerr << "\n";
			}
			std::cerr << "-\n";
#endif

			/* at this stage, we have a sequence of polygons. the first
				 is the "outside edge' or 'body' or 'border', and the rest of the
				 polygons are 'holes' within the first. there are several
				 options here to get rid of the holes. we choose to go ahead
				 and let the tessellater deal with the holes, and then
				 just output the resulting 3d triangles*/

			// We cannot trust the plane from Nef polyhedron to be correct.
			// Passing an incorrect normal vector can cause a crash in the constrained delaunay triangulator
      // See http://cgal-discuss.949826.n4.nabble.com/Nef3-Wrong-normal-vector-reported-causes-triangulator-crash-tt4660282.html
			// CGAL::Vector_3<CGAL_Kernel3> nvec = plane.orthogonal_vector();
			// K::Vector_3 normal(CGAL::to_double(nvec.x()), CGAL::to_double(nvec.y()), CGAL::to_double(nvec.z()));
			std::vector<IndexedTriangle> triangles;
			bool err = GeometryUtils::tessellatePolygonWithHoles(polyhole, triangles, NULL);
			const Vector3f *verts = &polyhole.vertices.front();
			if (!err) {
				BOOST_FOREACH(const Vector3i &t, triangles) {
					ps.append_poly();
					ps.append_vertex(verts[t[0]]);
					ps.append_vertex(verts[t[1]]);
					ps.append_vertex(verts[t[2]]);
				}
			}
		}
		return err;
	}
#endif
	CGAL_Nef_polyhedron *createNefPolyhedronFromGeometry(const Geometry &geom)
	{
		const PolySet *ps = dynamic_cast<const PolySet*>(&geom);
		if (ps) {
			return createNefPolyhedronFromPolySet(*ps);
		}
		else {
			const Polygon2d *poly2d = dynamic_cast<const Polygon2d*>(&geom);
			if (poly2d) return createNefPolyhedronFromPolygon2d(*poly2d);
		}
		assert(false && "createNefPolyhedronFromGeometry(): Unsupported geometry type");
		return NULL;
	}
}; // namespace CGALUtils


void ZRemover::visit(CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet)
{
	PRINTDB(" <!-- ZRemover Halffacet visit. Mark: %i --> ",hfacet->mark());
	if (hfacet->plane().orthogonal_direction() != this->up) {
		PRINTD("  <!-- ZRemover down-facing half-facet. skipping -->");
		PRINTD(" <!-- ZRemover Halffacet visit end-->");
		return;
	}

	// possible optimization - throw out facets that are vertically oriented

	CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator fci;
	int contour_counter = 0;
	CGAL_forall_facet_cycles_of(fci, hfacet) {
		if (fci.is_shalfedge()) {
			PRINTD(" <!-- ZRemover Halffacet cycle begin -->");
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(fci), cend(c1);
			std::vector<CGAL_Nef_polyhedron2::Explorer::Point> contour;
			CGAL_For_all(c1, cend) {
				CGAL_Nef_polyhedron3::Point_3 point3d = c1->source()->target()->point();
				CGAL_Nef_polyhedron2::Explorer::Point point2d(CGAL::to_double(point3d.x()),
																											CGAL::to_double(point3d.y()));
				contour.push_back(point2d);
			}
			if (contour.size()==0) continue;

			if (OpenSCAD::debug!="")
				PRINTDB(" <!-- is_simple_2: %i -->", CGAL::is_simple_2(contour.begin(), contour.end()));

			tmpnef2d.reset(new CGAL_Nef_polyhedron2(contour.begin(), contour.end(), boundary));

			if (contour_counter == 0) {
				PRINTDB(" <!-- contour is a body. make union(). %i  points -->", contour.size());
				*(output_nefpoly2d) += *(tmpnef2d);
			} else {
				PRINTDB(" <!-- contour is a hole. make intersection(). %i  points -->", contour.size());
				*(output_nefpoly2d) *= *(tmpnef2d);
			}

			/*log << "\n<!-- ======== output tmp nef: ==== -->\n"
				<< OpenSCAD::dump_svg(*tmpnef2d) << "\n"
				<< "\n<!-- ======== output accumulator: ==== -->\n"
				<< OpenSCAD::dump_svg(*output_nefpoly2d) << "\n";*/

			contour_counter++;
		} else {
			PRINTD(" <!-- ZRemover trivial facet cycle skipped -->");
		}
		PRINTD(" <!-- ZRemover Halffacet cycle end -->");
	}
	PRINTD(" <!-- ZRemover Halffacet visit end -->");
}


#endif /* ENABLE_CGAL */

