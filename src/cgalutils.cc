#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "Polygon2d.h"
#include "polyset-utils.h"
#include "grid.h"
#include "node.h"

#include "cgal.h"
#include <CGAL/convex_hull_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>
#include "svg.h"
#include "Reindexer.h"

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

namespace CGALUtils {

	bool applyHull(const Geometry::ChildList &children, PolySet &result)
	{
		typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
		// Collect point cloud
		std::set<K::Point_3> points;

		BOOST_FOREACH(const Geometry::ChildItem &item, children) {
			const shared_ptr<const Geometry> &chgeom = item.second;
			const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(chgeom.get());
			if (N) {
				for (CGAL_Nef_polyhedron3::Vertex_const_iterator i = N->p3->vertices_begin(); i != N->p3->vertices_end(); ++i) {
					points.insert(K::Point_3(to_double(i->point()[0]),to_double(i->point()[1]),to_double(i->point()[2])));
				}
			} else {
				const PolySet *ps = dynamic_cast<const PolySet *>(chgeom.get());
				if (ps) {
					BOOST_FOREACH(const PolySet::Polygon &p, ps->polygons) {
						BOOST_FOREACH(const Vector3d &v, p) {
							points.insert(K::Point_3(v[0], v[1], v[2]));
						}
					}
				}
			}
		}

		if (points.size() <= 3) return false;

		// Apply hull
		if (points.size() >= 4) {
			CGAL::Polyhedron_3<K> r;
			CGAL::convex_hull_3(points.begin(), points.end(), r);
			if (!createPolySetFromPolyhedron(r, result))
				return true;
			return false;
		} else {
			return false;
		}
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

				typedef CGAL::Exact_predicates_inexact_constructions_kernel Hull_kernel;

				std::list<CGAL_Polyhedron> P[2];
				std::list<CGAL::Polyhedron_3<Hull_kernel> > result_parts;

				for (int i = 0; i < 2; i++) {
					CGAL_Polyhedron poly;

					const PolySet * ps = dynamic_cast<const PolySet *>(operands[i]);

					const CGAL_Nef_polyhedron * nef = dynamic_cast<const CGAL_Nef_polyhedron *>(operands[i]);

					if (ps) createPolyhedronFromPolySet(*ps, poly);
					else if (nef && nef->p3->is_simple()) nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>(*nef->p3, poly);
					else throw 0;

					if (ps && ps->is_convex() || !ps && is_weakly_convex(poly)) {
						PRINTDB("Minkowski: child %d is convex and %s",i % (ps?"PolySet":"Nef") );
						P[i].push_back(poly);
					} else {
						CGAL_Nef_polyhedron3 decomposed_nef;

						if (ps) {
							PRINTDB("Minkowski: child %d is nonconvex PolySet, transforming to Nef and decomposing...", i);
							CGAL_Nef_polyhedron *p = createNefPolyhedronFromGeometry(*ps);
							decomposed_nef = *p->p3;
							delete p;
						} else {
							PRINTDB("Minkowski: child %d is nonconvex Nef, decomposing...",i);
							decomposed_nef = *nef->p3;
						}

						CGAL::convex_decomposition_3(decomposed_nef);

						// the first volume is the outer volume, which ignored in the decomposition
						CGAL_Nef_polyhedron3::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
						for( ; ci != decomposed_nef.volumes_end(); ++ci) {
							if(ci->mark()) {
								CGAL_Polyhedron poly;
								decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
								P[i].push_back(poly);
							}
						}


						PRINTDB("Minkowski: decomposed into %d convex parts", P[i].size());
					}
				}

				std::vector<Hull_kernel::Point_3> points[2];
				std::vector<Hull_kernel::Point_3> minkowski_points;

				for (int i = 0; i < P[0].size(); i++) {
					for (int j = 0; j < P[1].size(); j++) {
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
					CGAL_Nef_polyhedron *N = new CGAL_Nef_polyhedron;
					CGALUtils::applyOperator(fake_children, *N, OPENSCAD_UNION);
					t.stop();
					PRINTDB("Minkowski: Union done: %f s",t.time());
					t.reset();
					operands[0] = N;
				} else {
					return NULL;
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

			CGAL_Nef_polyhedron *N = new CGAL_Nef_polyhedron;
			applyOperator(children, *N, OPENSCAD_MINKOWSKI);
			return N;
		}
	}
	
/*!
	Applies op to all children and stores the result in dest.
	The child list should be guaranteed to contain non-NULL 3D or empty Geometry objects
*/
	void applyOperator(const Geometry::ChildList &children, CGAL_Nef_polyhedron &dest, OpenSCADOperator op)
	{
		// Speeds up n-ary union operations significantly
		CGAL::Nef_nary_union_3<CGAL_Nef_polyhedron3> nary_union;
		int nary_union_num_inserted = 0;
		CGAL_Nef_polyhedron *N = NULL;

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
					CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
					try {
						nary_union.add_polyhedron(*chN->p3);
						nary_union_num_inserted++;
					}
					catch (const CGAL::Failure_exception &e) {
						PRINTB("CGAL error in CGALUtils::applyBinaryOperator union: %s", e.what());
					}
					CGAL::set_error_behaviour(old_behaviour);
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

			CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
			try {
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
			}
			catch (const CGAL::Failure_exception &e) {
				// union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
				std::string opstr = op == OPENSCAD_INTERSECTION ? "intersection" : op == OPENSCAD_DIFFERENCE ? "difference" : op == OPENSCAD_MINKOWSKI ? "minkowski" : "UNKNOWN";
				PRINTB("CGAL error in CGALUtils::applyBinaryOperator %s: %s", opstr % e.what());
				
				// Errors can result in corrupt polyhedrons, so put back the old one
				*N = *chN;
			}
			CGAL::set_error_behaviour(old_behaviour);
			item.first->progress_report();
		}

		if (op == OPENSCAD_UNION && nary_union_num_inserted > 0) {
			CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
			try {

				N = new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(nary_union.get_union()));

			} catch (const CGAL::Failure_exception &e) {
				std::string opstr = "union";
				PRINTB("CGAL error in CGALUtils::applyBinaryOperator %s: %s", opstr % e.what());
			}
			CGAL::set_error_behaviour(old_behaviour);
		}
		if (N) dest = *N;
	}

/*!
	Modifies target by applying op to target and src:
	target = target [op] src
*/
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
			PRINTB("CGAL error in CGALUtils::applyBinaryOperator %s: %s", opstr % e.what());

			// Errors can result in corrupt polyhedrons, so put back the old one
			target = src;
		}
		CGAL::set_error_behaviour(old_behaviour);
	}

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
				PRINTB("CGALUtils::project during plane intersection: %s", e.what());
				try {
					PRINT("Trying alternative intersection using very large thin box: ");
					std::vector<CGAL_Point_3> pts;
					// dont use z of 0. there are bugs in CGAL.
					double inf = 1e8;
					double eps = 0.001;
					CGAL_Point_3 minpt( -inf, -inf, -eps );
					CGAL_Point_3 maxpt(  inf,  inf,  eps );
					CGAL_Iso_cuboid_3 bigcuboid( minpt, maxpt );
					for ( int i=0;i<8;i++ ) pts.push_back( bigcuboid.vertex(i) );
					CGAL_Polyhedron bigbox;
					CGAL::convex_hull_3(pts.begin(), pts.end(), bigbox);
					CGAL_Nef_polyhedron3 nef_bigbox( bigbox );
					newN.p3.reset(new CGAL_Nef_polyhedron3(nef_bigbox.intersection(*N.p3)));
				}
				catch (const CGAL::Failure_exception &e) {
					PRINTB("CGAL error in CGALUtils::project during bigbox intersection: %s", e.what());
				}
			}
				
			if (!newN.p3 || newN.p3->is_empty()) {
				CGAL::set_error_behaviour(old_behaviour);
				PRINT("WARNING: projection() failed.");
				return poly;
			}
				
			PRINTDB("%s",OpenSCAD::svg_header( 480, 100000 ));
			try {
				ZRemover zremover;
				CGAL_Nef_polyhedron3::Volume_const_iterator i;
				CGAL_Nef_polyhedron3::Shell_entry_const_iterator j;
				CGAL_Nef_polyhedron3::SFace_const_handle sface_handle;
				for ( i = newN.p3->volumes_begin(); i != newN.p3->volumes_end(); ++i ) {
					PRINTDB("<!-- volume. mark: %s -->",i->mark());
					for ( j = i->shells_begin(); j != i->shells_end(); ++j ) {
						PRINTDB("<!-- shell. (vol mark was: %i)", i->mark());;
						sface_handle = CGAL_Nef_polyhedron3::SFace_const_handle( j );
						newN.p3->visit_shell_objects( sface_handle , zremover );
						PRINTD("<!-- shell. end. -->");
					}
					PRINTD("<!-- volume end. -->");
				}
				poly = convertToPolygon2d(*zremover.output_nefpoly2d);
			}	catch (const CGAL::Failure_exception &e) {
				PRINTB("CGAL error in CGALUtils::project while flattening: %s", e.what());
			}
			PRINTD("</svg>");
				
			CGAL::set_error_behaviour(old_behaviour);
		}
		// In projection mode all the triangles are projected manually into the XY plane
		else {
			PolySet *ps3 = N.convertToPolyset();
			if (!ps3) return poly;
			poly = PolysetUtils::project(*ps3);
			delete ps3;
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
		if (points.size()) result = CGAL::bounding_box( points.begin(), points.end() );
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
			size_t N = ps.polygons[i].size();
			assert(N > 0);
			std::vector<Point> v(N);
			for (int j = 0; j < N; j++) {
				v[j] = vector_convert<Point>(ps.polygons[i][j]);
				Edge edge(ps.polygons[i][j],ps.polygons[i][(j+1)%N]);
				if (edge_to_facet_map.count(edge)) return false; // edge already exists: nonmanifold
				edge_to_facet_map[edge] = i;
			}
			Vector normal;
			CGAL::normal_vector_newell_3(v.begin(), v.end(), normal);

			facet_planes.push_back(Plane(v[0], normal));
		}

		for (int i = 0; i < ps.polygons.size(); i++) {
			size_t N = ps.polygons[i].size();
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
				Edge_to_facet_map::iterator it = edge_to_facet_map.find(Edge(ps.polygons[f][i], ps.polygons[f][j]));
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
};

template <typename Polyhedron>
bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps)
{
	bool err = false;
	typedef typename Polyhedron::Vertex                                 Vertex;
	typedef typename Polyhedron::Vertex_const_iterator                  VCI;
	typedef typename Polyhedron::Facet_const_iterator                   FCI;
	typedef typename Polyhedron::Halfedge_around_facet_const_circulator HFCC;
		
	for (FCI fi = p.facets_begin(); fi != p.facets_end(); ++fi) {
		HFCC hc = fi->facet_begin();
		HFCC hc_end = hc;
		ps.append_poly();
		do {
			Vertex const& v = *((hc++)->vertex());
			double x = CGAL::to_double(v.point().x());
			double y = CGAL::to_double(v.point().y());
			double z = CGAL::to_double(v.point().z());
			ps.append_vertex(x, y, z);
		} while (hc != hc_end);
	}
	return err;
}

template bool createPolySetFromPolyhedron(const CGAL_Polyhedron &p, PolySet &ps);
template bool createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Epick> &p, PolySet &ps);
template bool createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Epeck> &p, PolySet &ps);
template bool createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Simple_cartesian<long> > &p, PolySet &ps);


/////// Tessellation begin

typedef CGAL::Plane_3<CGAL_Kernel3> CGAL_Plane_3;


/*

This is our custom tessellator of Nef Polyhedron faces. The problem with 
Nef faces is that sometimes the 'default' tessellator of Nef Polyhedron 
doesnt work. This is particularly true with situations where the polygon 
face is not, actually, 'simple', according to CGAL itself. This can 
occur on a bad quality STL import but also for other reasons. The 
resulting Nef face will appear to the average human eye as an ordinary, 
simple polygon... but in reality it has multiple edges that are 
slightly-out-of-alignment and sometimes they backtrack on themselves.

When the triangulator is fed a polygon with self-intersecting edges, 
it's default behavior is to throw an exception. The other terminology 
for this is to say that the 'constraints' in the triangulation are 
'intersecting'. The 'constraints' represent the edges of the polygon. 
The 'triangulation' is the covering of all the polygon points with 
triangles.

How do we allow interseting constraints during triangulation? We use an 
'Itag' for the triangulation, per the CGAL docs. This allows the 
triangulator to run without throwing an exception when it encounters 
self-intersecting polygon edges. The trick here is that when it finds
an intersection, it actually creates a new point. 

The triangulator creates new points in 2d, but they aren't matched to 
any 3d points on our 3d polygon plane. (The plane of the Nef face). How 
to fix this problem? We actually 'project back up' or 'lift' into the 3d 
plane from the 2d point. This is handled in the 'deproject()' function.

There is also the issue of the Simplicity of Nef Polyhedron face 
polygons. They are often not simple. The intersecting-constraints 
Triangulation can triangulate non-simple polygons, but of course it's 
result is also non-simple. This means that CGAL functions like 
orientation_2() and bounded_side() simply will not work on the resulting 
polygons because they all require input polygons to pass the 
'is_simple2()' test. We have to use alternatives in order to create our
triangles.

There is also the question of which underlying number type to use. Some 
of the CGAL functions simply dont guarantee good results with a type 
like double. Although much the math here is somewhat simple, like 
line-line intersection, and involves only simple algebra, the 
approximations required when using floating-point types can cause the 
answers to be wrong. For example questions like 'is a point inside a 
triangle' do not have good answers under floating-point systems where a 
line may have a slope that is not expressible exactly as a floating 
point number. There are ways to deal with floating point inaccuracy but 
it is much, much simpler to use Rational numbers, although potentially
much slower in many cases.

*/

#include <CGAL/Delaunay_mesher_no_edge_refinement_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>

typedef CGAL_Kernel3 Kernel;
//typedef CGAL::Triangulation_vertex_base_2<Kernel> Vb;
typedef CGAL::Triangulation_vertex_base_2<Kernel> Vb;
//typedef CGAL::Constrained_triangulation_face_base_2<Kernel> Fb;
typedef CGAL::Delaunay_mesh_face_base_2<Kernel> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb> TDS;
typedef CGAL::Exact_intersections_tag ITAG;
typedef CGAL::Constrained_Delaunay_triangulation_2<Kernel,TDS,ITAG> CDT;
//typedef CGAL::Constrained_Delaunay_triangulation_2<Kernel,TDS> CDT;

typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Point CDTPoint;

typedef CGAL::Ray_2<Kernel> CGAL_Ray_2;
typedef CGAL::Line_3<Kernel> CGAL_Line_3;
typedef CGAL::Point_2<Kernel> CGAL_Point_2;
typedef CGAL::Vector_2<Kernel> CGAL_Vector_2;
typedef CGAL::Segment_2<Kernel> CGAL_Segment_2;
typedef std::vector<CGAL_Point_3> CGAL_Polygon_3;
typedef CGAL::Direction_2<Kernel> CGAL_Direction_2;
typedef CGAL::Direction_3<Kernel> CGAL_Direction_3;

/* The idea of 'projection' is how we make 3d points appear as though 
they were 2d points to the tessellation algorithm. We take the 3-d plane 
on which the polygon lies, and then 'project' or 'cast its shadow' onto 
one of three standard planes, the xyplane, the yzplane, or the xzplane, 
depending on which projection will prevent the polygon looking like a 
flat line. (imagine, the triangle 0,0,1 0,1,1 0,1,0 ... if viewed from 
the 'top' it looks line a flat line. so we want to view it from the 
side). Thus we create a sequence of x,y points to feed to the algorithm,
but those points might actually be x,z pairs or y,z pairs... it is an
illusion we present to the triangulation algorithm by way of 'projection'.
We get a resulting sequence of triangles with x,y coordinates, which we
then 'deproject' back to x,z or y,z, in 3d space as needed. For example
the square 0,0,0 0,0,1 0,1,1 0,1,0 becomes '0,0 0,1 1,1 1,0', is then
split into two triangles, 0,0 1,0 1,1 and 0,0 1,1 0,1. those two triangles
then are projected back to 3d as 0,0,0 0,1,0 0,1,1 and 0,0 0,1,1 0,0,1. 

There is an additional trick we do with projection related to Polygon 
orientation and the orientation of our output triangles, and thus, which 
way they are facing in space (aka their 'normals' or 'oriented side'). 

The basic issues is this: every 3d flat polygon can be thought of as 
having two sides. In Computer Graphics the convention is that the 
'outside' or 'oriented side' or 'normal' is determined by looking at the 
triangle in terms of the 'ordering' or 'winding' of the points. If the 
points come in a 'clockwise' order, you must be looking at the triangle 
from 'inside'. If the points come in a 'counterclockwise' order, you 
must be looking at the triangle from the outside. For example, the 
triangle 0,0,0 1,0,0 0,1,0, when viewed from the 'top', has points in a 
counterclockwise order, so the 'up' side is the 'normal' or 'outside'.
if you look at that same triangle from the 'bottom' side, the points
will appear to be 'clockwise', so the 'down' side is the 'inside', and is the 
opposite of the 'normal' side.

How do we keep track of all that when doing a triangulation? We could
check each triangle as it was generated, and fix it's orientation before
we feed it back to our output list. That is done by, for example, checking
the orientation of the input polygon and then forcing the triangle to 
match that orientation during output. This is what CGAL's Nef Polyhedron
does, you can read it inside /usr/include/CGAL/Nef_polyhedron_3.h.

Or.... we could actually add an additional 'projection' to the incoming 
polygon points so that our triangulation algorithm is guaranteed to 
create triangles with the proper orientation in the first place. How? 
First, we assume that the triangulation algorithm will always produce 
'counterclockwise' triangles in our plain old x-y plane.

The method is based on the following curious fact: That is, if you take 
the points of a polygon, and flip the x,y coordinate of each point, 
making y:=x and x:=y, then you essentially get a 'mirror image' of the 
original polygon... but the orientation will be flipped. Given a 
clockwise polygon, the 'flip' will result in a 'counterclockwise' 
polygon mirror-image and vice versa. 

Now, there is a second curious fact that helps us here. In 3d, we are 
using the plane equation of ax+by+cz+d=0, where a,b,c determine its 
direction. If you notice, there are actually mutiple sets of numbers 
a:b:c that will describe the exact same plane. For example the 'ground' 
plane, called the XYplane, where z is everywhere 0, has the equation 
0x+0y+1z+0=0, simplifying to a solution for x,y,z of z=0 and x,y = any 
numbers in your number system. However you can also express this as 
0x+0y+-1z=0. The x,y,z solution is the same: z is everywhere 0, x and y 
are any number, even though a,b,c are different. We can say that the 
plane is 'oriented' differently, if we wish.

But how can we link that concept to the points on the polygon? Well, if 
you generate a plane using the standard plane-equation generation 
formula, given three points M,N,P, then you will get a plane equation 
with <a:b:c:d>. However if you feed the points in the reverse order, 
P,N,M, so that they are now oriented in the opposite order, you will get 
a plane equation with the signs flipped. <-a:-b:-c:-d> This means you 
can essentially consider that a plane has an 'orientation' based on it's 
equation, by looking at the signs of a,b,c relative to some other 
quantity.

This means that you can 'flip' the projection of the input polygon 
points so that the projection will match the orientation of the input 
plane, thus guaranteeing that the output triangles will be oriented in 
the same direction as the input polygon was. In other words, even though 
we technically 'lose information' when we project from 3d->2d, we can 
actually keep the concept of 'orientation' through the whole 
triangulation process, and not have to recalculate the proper 
orientation during output.

For example take two side-squares of a cube and the plane equations 
formed by feeding the points in counterclockwise, as if looking in from 
outside the cube:

 0,0,0 0,1,0 0,1,1 0,0,1     <-1:0:0:0>
 1,0,0 1,1,0 1,1,1 1,0,1      <1:0:0:1>

They are both projected onto the YZ plane. They look the same:
  0,0 1,0 1,1 0,1
  0,0 1,0 1,1 0,1

But the second square plane has opposite orientation, so we flip the x 
and y for each point:
  0,0 1,0 1,1 0,1
  0,0 0,1 1,1 1,0

Only now do we feed these two 2-d squares to the tessellation algorithm. 
The result is 4 triangles. When de-projected back to 3d, they will have 
the appropriate winding that will match that of the original 3d faces.
And the first two triangles will have opposite orientation from the last two.
*/

typedef enum { XYPLANE, YZPLANE, XZPLANE, NONE } plane_t;
struct projection_t {
	plane_t plane;
	bool flip;
};

CGAL_Point_2 get_projected_point( CGAL_Point_3 &p3, projection_t projection ) {
	NT3 x,y;
	if      (projection.plane == XYPLANE) { x = p3.x(); y = p3.y(); }
	else if (projection.plane == XZPLANE) { x = p3.x(); y = p3.z(); }
	else if (projection.plane == YZPLANE) { x = p3.y(); y = p3.z(); }
	else if (projection.plane == NONE) { x = 0; y = 0; }
	if (projection.flip) return CGAL_Point_2( y,x );
	return CGAL_Point_2( x,y );
}

/* given 2d point, 3d plane, and 3d->2d projection, 'deproject' from
 2d back onto a point on the 3d plane. true on failure, false on success */
bool deproject( CGAL_Point_2 &p2, projection_t &projection, CGAL_Plane_3 &plane, CGAL_Point_3 &p3 )
{
	NT3 x,y;
	CGAL_Line_3 l;
	CGAL_Point_3 p;
	CGAL_Point_2 pf( p2.x(), p2.y() );
	if (projection.flip) pf = CGAL_Point_2( p2.y(), p2.x() );
        if (projection.plane == XYPLANE) {
		p = CGAL_Point_3( pf.x(), pf.y(), 0 );
		l = CGAL_Line_3( p, CGAL_Direction_3(0,0,1) );
	} else if (projection.plane == XZPLANE) {
		p = CGAL_Point_3( pf.x(), 0, pf.y() );
		l = CGAL_Line_3( p, CGAL_Direction_3(0,1,0) );
	} else if (projection.plane == YZPLANE) {
		p = CGAL_Point_3( 0, pf.x(), pf.y() );
		l = CGAL_Line_3( p, CGAL_Direction_3(1,0,0) );
	}
	CGAL::Object obj = CGAL::intersection( l, plane );
	const CGAL_Point_3 *point_test = CGAL::object_cast<CGAL_Point_3>(&obj);
	if (point_test) {
		p3 = *point_test;
		return false;
	}
	PRINT("ERROR: deproject failure");
	return true;
}

/* this simple criteria guarantees CGALs triangulation algorithm will
terminate (i.e. not lock up and freeze the program) */
template <class T> class DummyCriteria {
public:
        typedef double Quality;
        class Is_bad {
        public:
                CGAL::Mesh_2::Face_badness operator()(const Quality) const {
                        return CGAL::Mesh_2::NOT_BAD;
                }
                CGAL::Mesh_2::Face_badness operator()(const typename T::Face_handle&, Quality&q) const {
                        q = 1;
                        return CGAL::Mesh_2::NOT_BAD;
                }
        };
        Is_bad is_bad_object() const { return Is_bad(); }
};

NT3 sign( const NT3 &n )
{
	if (n>0) return NT3(1);
	if (n<0) return NT3(-1);
	return NT3(0);
}

/* wedge, also related to 'determinant', 'signed parallelogram area', 
'side', 'turn', 'winding', '2d portion of cross-product', etc etc. this 
function can tell you whether v1 is 'counterclockwise' or 'clockwise' 
from v2, based on the sign of the result. when the input Vectors are 
formed from three points, A-B and B-C, it can tell you if the path along 
the points A->B->C is turning left or right.*/
NT3 wedge( CGAL_Vector_2 &v1, CGAL_Vector_2 &v2 ) {
	return v1.x()*v2.y()-v2.x()*v1.y();
}

/* given a point and a possibly non-simple polygon, determine if the
point is inside the polygon or not, using the given winding rule. note
that even_odd is not implemented. */
typedef enum { NONZERO_WINDING, EVEN_ODD } winding_rule_t;
bool inside(CGAL_Point_2 &p1,std::vector<CGAL_Point_2> &pgon, winding_rule_t winding_rule)
{
	NT3 winding_sum = NT3(0);
	CGAL_Point_2 p2;
	CGAL_Ray_2 eastray(p1,CGAL_Direction_2(1,0));
	for (size_t i=0;i<pgon.size();i++) {
		CGAL_Point_2 tail = pgon[i];
		CGAL_Point_2 head = pgon[(i+1)%pgon.size()];
		CGAL_Segment_2 seg( tail, head );
		CGAL::Object obj = intersection( eastray, seg );
		const CGAL_Point_2 *point_test = CGAL::object_cast<CGAL_Point_2>(&obj);
		if (point_test) {
			p2 = *point_test;
			CGAL_Vector_2 v1( p1, p2 );
			CGAL_Vector_2 v2( p2, head );
			NT3 this_winding = wedge( v1, v2 );
			winding_sum += sign(this_winding);
		} else {
			continue;
		}
	}
	if (winding_sum != NT3(0) && winding_rule == NONZERO_WINDING ) return true;
	return false;
}

projection_t find_good_projection( CGAL_Plane_3 &plane )
{
	projection_t goodproj;
	goodproj.plane = NONE;
	goodproj.flip = false;
        NT3 qxy = plane.a()*plane.a()+plane.b()*plane.b();
        NT3 qyz = plane.b()*plane.b()+plane.c()*plane.c();
        NT3 qxz = plane.a()*plane.a()+plane.c()*plane.c();
        NT3 min = std::min(qxy,std::min(qyz,qxz));
        if (min==qxy) {
		goodproj.plane = XYPLANE;
		if (sign(plane.c())>0) goodproj.flip = true;
	} else if (min==qyz) {
		goodproj.plane = YZPLANE;
		if (sign(plane.a())>0) goodproj.flip = true;
	} else if (min==qxz) {
		goodproj.plane = XZPLANE;
		if (sign(plane.b())<0) goodproj.flip = true;
	} else PRINT("ERROR: failed to find projection");
	return goodproj;
}

/* given a single near-planar 3d polygon with holes, tessellate into a 
sequence of polygons without holes. as of writing, this means conversion 
into a sequence of 3d triangles. the given plane should be the same plane
holding the polygon and it's holes. */
bool tessellate_3d_face_with_holes( std::vector<CGAL_Polygon_3> &polygons, std::vector<CGAL_Polygon_3> &triangles, CGAL_Plane_3 &plane )
{
	if (polygons.size()==1 && polygons[0].size()==3) {
		PRINT("input polygon has 3 points. shortcut tessellation.");
		CGAL_Polygon_3 t;
		t.push_back(polygons[0][2]);
		t.push_back(polygons[0][1]);
		t.push_back(polygons[0][0]);
		triangles.push_back( t );
		return false;
	}
	bool err = false;
	CDT cdt;
	std::map<CDTPoint,CGAL_Point_3> vertmap;

	PRINT("finding good projection");
	projection_t goodproj = find_good_projection( plane );

	PRINTB("plane %s",plane );
	PRINTB("proj: %i %i",goodproj.plane % goodproj.flip);
	PRINT("Inserting points and edges into Constrained Delaunay Triangulation");
	std::vector< std::vector<CGAL_Point_2> > polygons2d;
	for (size_t i=0;i<polygons.size();i++) {
	        std::vector<Vertex_handle> vhandles;
		std::vector<CGAL_Point_2> polygon2d;
		for (size_t j=0;j<polygons[i].size();j++) {
			CGAL_Point_3 p3 = polygons[i][j];
			CGAL_Point_2 p2 = get_projected_point( p3, goodproj );
			CDTPoint cdtpoint = CDTPoint( p2.x(), p2.y() );
			vertmap[ cdtpoint ] = p3;
			Vertex_handle vh = cdt.push_back( cdtpoint );
			vhandles.push_back( vh );
			polygon2d.push_back( p2 );
		}
		polygons2d.push_back( polygon2d );
		for (size_t k=0;k<vhandles.size();k++) {
			int vindex1 = (k+0);
			int vindex2 = (k+1)%vhandles.size();
			CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
			try {
				cdt.insert_constraint( vhandles[vindex1], vhandles[vindex2] );
			} catch (const CGAL::Failure_exception &e) {
				PRINTB("WARNING: Constraint insertion failure %s", e.what());
			}
			CGAL::set_error_behaviour(old_behaviour);
		}
	}

	size_t numholes = polygons2d.size()-1;
	PRINTDB("seeding %i holes",numholes);
	std::list<CDTPoint> list_of_seeds;
	for (size_t i=1;i<polygons2d.size();i++) {
		std::vector<CGAL_Point_2> &pgon = polygons2d[i];
		for (size_t j=0;j<pgon.size();j++) {
			CGAL_Point_2 p1 = pgon[(j+0)];
			CGAL_Point_2 p2 = pgon[(j+1)%pgon.size()];
			CGAL_Point_2 p3 = pgon[(j+2)%pgon.size()];
			CGAL_Point_2 mp = CGAL::midpoint(p1,CGAL::midpoint(p2,p3));
			if (inside(mp,pgon,NONZERO_WINDING)) {
				CDTPoint cdtpt( mp.x(), mp.y() );
				list_of_seeds.push_back( cdtpt );
				break;
			}
		}
	}
	std::list<CDTPoint>::iterator li = list_of_seeds.begin();
	for (;li!=list_of_seeds.end();li++) {
		//PRINTB("seed %s",*li);
		double x = CGAL::to_double( li->x() );
		double y = CGAL::to_double( li->y() );
		PRINTDB("seed %f,%f",x%y);
	}
	PRINTD("seeding done");

	PRINTD( "meshing" );
	CGAL::refine_Delaunay_mesh_2_without_edge_refinement( cdt,
		list_of_seeds.begin(), list_of_seeds.end(),
		DummyCriteria<CDT>() );

	PRINTD("meshing done");
	// this fails because it calls is_simple and is_simple fails on many
	// Nef Polyhedron faces
	//CGAL::Orientation original_orientation =
	//	CGAL::orientation_2( orienpgon.begin(), orienpgon.end() );

	CDT::Finite_faces_iterator fit;
	for( fit=cdt.finite_faces_begin(); fit!=cdt.finite_faces_end(); fit++ )
	{
		if(fit->is_in_domain()) {
			CDTPoint p1 = cdt.triangle( fit )[0];
			CDTPoint p2 = cdt.triangle( fit )[1];
			CDTPoint p3 = cdt.triangle( fit )[2];
			CGAL_Point_3 cp1,cp2,cp3;
			CGAL_Polygon_3 pgon;
			if (vertmap.count(p1)) cp1 = vertmap[p1];
			else err = deproject( p1, goodproj, plane, cp1 );
			if (vertmap.count(p2)) cp2 = vertmap[p2];
			else err = deproject( p2, goodproj, plane, cp2 );
			if (vertmap.count(p3)) cp3 = vertmap[p3];
			else err = deproject( p3, goodproj, plane, cp3 );
			if (err) PRINT("WARNING: 2d->3d deprojection failure");
			pgon.push_back( cp1 );
			pgon.push_back( cp2 );
			pgon.push_back( cp3 );
                        triangles.push_back( pgon );
                }
        }

	PRINTDB("built %i triangles",triangles.size());
	return err;
}
/////// Tessellation end

/*
	Create a PolySet from a Nef Polyhedron 3. return false on success, 
	true on failure. The trick to this is that Nef Polyhedron3 faces have 
	'holes' in them. . . while PolySet (and many other 3d polyhedron 
	formats) do not allow for holes in their faces. The function documents 
	the method used to deal with this
*/
bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
{
	bool err = false;
	CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
	CGAL_forall_halffacets( hfaceti, N ) {
		CGAL_Plane_3 plane( hfaceti->plane() );
		std::vector<CGAL_Polygon_3> polygons;
		// the 0-mark-volume is the 'empty' volume of space. skip it.
		if (hfaceti->incident_volume()->mark() == 0) continue;
		CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
		CGAL_forall_facet_cycles_of( cyclei, hfaceti ) {
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
			CGAL_Polygon_3 polygon;
			CGAL_For_all( c1, c2 ) {
				CGAL_Point_3 p = c1->source()->center_vertex()->point();
				polygon.push_back( p );
			}
			polygons.push_back( polygon );
		}

		/* at this stage, we have a sequence of polygons. the first
		is the "outside edge' or 'body' or 'border', and the rest of the
		polygons are 'holes' within the first. there are several
		options here to get rid of the holes. we choose to go ahead
		and let the tessellater deal with the holes, and then
		just output the resulting 3d triangles*/
		std::vector<CGAL_Polygon_3> triangles;
		bool err = tessellate_3d_face_with_holes( polygons, triangles, plane );
		if (!err) for (size_t i=0;i<triangles.size();i++) {
			if (triangles[i].size()!=3) {
				PRINT("WARNING: triangle doesn't have 3 points. skipping");
				continue;
			}
			ps.append_poly();
			for (size_t j=0;j<3;j++) {
				double x1,y1,z1;
				x1 = CGAL::to_double( triangles[i][j].x() );
				y1 = CGAL::to_double( triangles[i][j].y() );
				z1 = CGAL::to_double( triangles[i][j].z() );
				ps.append_vertex(x1,y1,z1);
			}
		}
	}
	return err;
}

#undef GEN_SURFACE_DEBUG

namespace Eigen {
	size_t hash_value(Vector3d const &v) {
		size_t seed = 0;
		for (int i=0;i<3;i++) boost::hash_combine(seed, v[i]);
		return seed;
	}
}

class CGAL_Build_PolySet : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	typedef CGAL_Polybuilder::Point_3 CGALPoint;

	const PolySet &ps;
	CGAL_Build_PolySet(const PolySet &ps) : ps(ps) { }

/*
	Using Grid here is important for performance reasons. See following model.
	If we don't grid the geometry before converting to a Nef Polyhedron, the quads
	in the cylinders to tessellated into triangles since floating point
	incertainty causes the faces to not be 100% planar. The incertainty is exaggerated
	by the transform. This wasn't a problem earlier since we used Nef for everything,
	but optimizations since then has made us keep it in floating point space longer.

  minkowski() {
    cube([200, 50, 7], center = true);
    rotate([90,0,0]) cylinder($fn = 8, h = 1, r = 8.36, center = true);
    rotate([0,90,0]) cylinder($fn = 8, h = 1, r = 8.36, center = true);
  }
 */
#if 1 // Use Grid
	void operator()(CGAL_HDS& hds) {
		CGAL_Polybuilder B(hds, true);
		
		std::vector<CGALPoint> vertices;
		Grid3d<int> grid(GRID_FINE);
		std::vector<size_t> indices(3);
		
		BOOST_FOREACH(const PolySet::Polygon &p, ps.polygons) {
			BOOST_REVERSE_FOREACH(Vector3d v, p) {
				if (!grid.has(v[0], v[1], v[2])) {
					// align v to the grid; the CGALPoint will receive the aligned vertex
					grid.align(v[0], v[1], v[2]) = vertices.size();
					vertices.push_back(CGALPoint(v[0], v[1], v[2]));
				}
			}
		}

#ifdef GEN_SURFACE_DEBUG
		printf("polyhedron(faces=[");
		int pidx = 0;
#endif
		B.begin_surface(vertices.size(), ps.polygons.size());
		BOOST_FOREACH(const CGALPoint &p, vertices) {
			B.add_vertex(p);
		}
		BOOST_FOREACH(const PolySet::Polygon &p, ps.polygons) {
#ifdef GEN_SURFACE_DEBUG
			if (pidx++ > 0) printf(",");
#endif
			indices.clear();
			BOOST_FOREACH(const Vector3d &v, p) {
				indices.push_back(grid.data(v[0], v[1], v[2]));
			}

			// We perform this test since there is a bug in CGAL's
			// Polyhedron_incremental_builder_3::test_facet() which
			// fails to detect duplicate indices
			bool err = false;
			for (std::size_t i = 0; i < indices.size(); ++i) {
        // check if vertex indices[i] is already in the sequence [0..i-1]
        for (std::size_t k = 0; k < i && !err; ++k) {
					if (indices[k] == indices[i]) {
						err = true;
						break;
					}
				}
			}
			if (!err && B.test_facet(indices.begin(), indices.end())) {
				B.add_facet(indices.begin(), indices.end());
			}
#ifdef GEN_SURFACE_DEBUG
				printf("[");
				int fidx = 0;
				BOOST_FOREACH(size_t i, indices) {
					if (fidx++ > 0) printf(",");
					printf("%ld", i);
				}
				printf("]");
#endif
		}
		B.end_surface();
#ifdef GEN_SURFACE_DEBUG
		printf("],\n");
#endif
#ifdef GEN_SURFACE_DEBUG
		printf("points=[");
		for (int i=0;i<vertices.size();i++) {
			if (i > 0) printf(",");
			const CGALPoint &p = vertices[i];
			printf("[%g,%g,%g]", CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z()));
		}
		printf("]);\n");
#endif
	}
#else // Don't use Grid
	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);
		typedef boost::tuple<double, double, double> BuilderVertex;
		Reindexer<Vector3d> vertices;
		std::vector<size_t> indices(3);

		// Estimating same # of vertices as polygons (very rough)
		B.begin_surface(ps.polygons.size(), ps.polygons.size());
		int pidx = 0;
#ifdef GEN_SURFACE_DEBUG
		printf("polyhedron(faces=[");
#endif
		BOOST_FOREACH(const PolySet::Polygon &p, ps.polygons) {
#ifdef GEN_SURFACE_DEBUG
			if (pidx++ > 0) printf(",");
#endif
			indices.clear();
			BOOST_REVERSE_FOREACH(const Vector3d &v, p) {
				size_t s = vertices.size();
				size_t idx = vertices.lookup(v);
				// If we added a vertex, also add it to the CGAL builder
				if (idx == s) B.add_vertex(CGALPoint(v[0], v[1], v[2]));
				indices.push_back(idx);
			}
			// We perform this test since there is a bug in CGAL's
			// Polyhedron_incremental_builder_3::test_facet() which
			// fails to detect duplicate indices
			bool err = false;
			for (std::size_t i = 0; i < indices.size(); ++i) {
        // check if vertex indices[i] is already in the sequence [0..i-1]
        for (std::size_t k = 0; k < i && !err; ++k) {
					if (indices[k] == indices[i]) {
						err = true;
						break;
					}
				}
			}
			if (!err && B.test_facet(indices.begin(), indices.end())) {
				B.add_facet(indices.begin(), indices.end());
#ifdef GEN_SURFACE_DEBUG
				printf("[");
				int fidx = 0;
				BOOST_FOREACH(size_t i, indices) {
					if (fidx++ > 0) printf(",");
					printf("%ld", i);
				}
				printf("]");
#endif
			}
		}
		B.end_surface();
#ifdef GEN_SURFACE_DEBUG
		printf("],\n");

		printf("points=[");
		for (int vidx=0;vidx<vertices.size();vidx++) {
			if (vidx > 0) printf(",");
			const Vector3d &v = vertices.getArray()[vidx];
			printf("[%g,%g,%g]", v[0], v[1], v[2]);
		}
		printf("]);\n");
#endif
	}
#endif
};

bool createPolyhedronFromPolySet(const PolySet &ps, CGAL_Polyhedron &p)
{
	bool err = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Build_PolySet builder(ps);
		p.delegate(builder);
	}
	catch (const CGAL::Assertion_exception &e) {
		PRINTB("CGAL error in CGALUtils::createPolyhedronFromPolySet: %s", e.what());
		err = true;
	}
	CGAL::set_error_behaviour(old_behaviour);
	return err;
}

void ZRemover::visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet )
{
	PRINTDB(" <!-- ZRemover Halffacet visit. Mark: %i --> ",hfacet->mark());
	if ( hfacet->plane().orthogonal_direction() != this->up ) {
		PRINTD("  <!-- ZRemover down-facing half-facet. skipping -->");
		PRINTD(" <!-- ZRemover Halffacet visit end-->");
		return;
	}

	// possible optimization - throw out facets that are vertically oriented

	CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator fci;
	int contour_counter = 0;
	CGAL_forall_facet_cycles_of( fci, hfacet ) {
		if ( fci.is_shalfedge() ) {
			PRINTD(" <!-- ZRemover Halffacet cycle begin -->");
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(fci), cend(c1);
			std::vector<CGAL_Nef_polyhedron2::Explorer::Point> contour;
			CGAL_For_all( c1, cend ) {
				CGAL_Nef_polyhedron3::Point_3 point3d = c1->source()->target()->point();
				CGAL_Nef_polyhedron2::Explorer::Point point2d(CGAL::to_double(point3d.x()),
																											CGAL::to_double(point3d.y()));
				contour.push_back( point2d );
			}
			if (contour.size()==0) continue;

			if (OpenSCAD::debug!="")
				PRINTDB(" <!-- is_simple_2: %i -->", CGAL::is_simple_2( contour.begin(), contour.end() ) );

			tmpnef2d.reset( new CGAL_Nef_polyhedron2( contour.begin(), contour.end(), boundary ) );

			if ( contour_counter == 0 ) {
				PRINTDB(" <!-- contour is a body. make union(). %i  points -->", contour.size() );
				*(output_nefpoly2d) += *(tmpnef2d);
			} else {
				PRINTDB(" <!-- contour is a hole. make intersection(). %i  points -->", contour.size() );
				*(output_nefpoly2d) *= *(tmpnef2d);
			}

			/*log << "\n<!-- ======== output tmp nef: ==== -->\n"
				<< OpenSCAD::dump_svg( *tmpnef2d ) << "\n"
				<< "\n<!-- ======== output accumulator: ==== -->\n"
				<< OpenSCAD::dump_svg( *output_nefpoly2d ) << "\n";*/

			contour_counter++;
		} else {
			PRINTD(" <!-- ZRemover trivial facet cycle skipped -->");
		}
		PRINTD(" <!-- ZRemover Halffacet cycle end -->");
	}
	PRINTD(" <!-- ZRemover Halffacet visit end -->");
}

namespace /* anonymous */ {
	// This code is from CGAL/demo/Polyhedron/Scene_nef_polyhedron_item.cpp
	// quick hacks to convert polyhedra from exact to inexact and vice-versa
	template <class Polyhedron_input,
	class Polyhedron_output>
	struct Copy_polyhedron_to
	: public CGAL::Modifier_base<typename Polyhedron_output::HalfedgeDS>
	{
		Copy_polyhedron_to(const Polyhedron_input& in_poly)
		: in_poly(in_poly) {}

		void operator()(typename Polyhedron_output::HalfedgeDS& out_hds)
		{
			typedef typename Polyhedron_output::HalfedgeDS Output_HDS;

			CGAL::Polyhedron_incremental_builder_3<Output_HDS> builder(out_hds);

			typedef typename Polyhedron_input::Vertex_const_iterator Vertex_const_iterator;
			typedef typename Polyhedron_input::Facet_const_iterator  Facet_const_iterator;
			typedef typename Polyhedron_input::Halfedge_around_facet_const_circulator HFCC;

			builder.begin_surface(in_poly.size_of_vertices(),
								  in_poly.size_of_facets(),
								  in_poly.size_of_halfedges());

			for(Vertex_const_iterator
				vi = in_poly.vertices_begin(), end = in_poly.vertices_end();
				vi != end ; ++vi)
			{
				typename Polyhedron_output::Point_3 p(::CGAL::to_double( vi->point().x()),
													  ::CGAL::to_double( vi->point().y()),
													  ::CGAL::to_double( vi->point().z()));
				builder.add_vertex(p);
			}

			typedef CGAL::Inverse_index<Vertex_const_iterator> Index;
			Index index( in_poly.vertices_begin(), in_poly.vertices_end());

			for(Facet_const_iterator
				fi = in_poly.facets_begin(), end = in_poly.facets_end();
				fi != end; ++fi)
			{
				HFCC hc = fi->facet_begin();
				HFCC hc_end = hc;
				//     std::size_t n = circulator_size( hc);
				//     CGAL_assertion( n >= 3);
				builder.begin_facet ();
				do {
					builder.add_vertex_to_facet(index[hc->vertex()]);
					++hc;
				} while( hc != hc_end);
				builder.end_facet();
			}
			builder.end_surface();
		} // end operator()(..)
	private:
		const Polyhedron_input& in_poly;
	}; // end Copy_polyhedron_to<>

	template <class Poly_A, class Poly_B>
	void copy_to(const Poly_A& poly_a, Poly_B& poly_b)
	{
		Copy_polyhedron_to<Poly_A, Poly_B> modifier(poly_a);
		poly_b.delegate(modifier);
	}
}

static CGAL_Nef_polyhedron *createNefPolyhedronFromPolySet(const PolySet &ps)
{
	if (ps.isEmpty()) return new CGAL_Nef_polyhedron();
	assert(ps.getDimension() == 3);

	if (ps.is_convex()) {
		typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
		// Collect point cloud
		std::set<K::Point_3> points;
		for (int i = 0; i < ps.polygons.size(); i++) {
			for (int j = 0; j < ps.polygons[i].size(); j++) {
				points.insert(vector_convert<K::Point_3>(ps.polygons[i][j]));
			}
		}

		if (points.size() <= 3) return new CGAL_Nef_polyhedron();;

		// Apply hull
		CGAL::Polyhedron_3<K> r;
		CGAL::convex_hull_3(points.begin(), points.end(), r);
		CGAL::Polyhedron_3<CGAL_Kernel3> r_exact;
		copy_to(r,r_exact);
		return new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(r_exact));
	}

	CGAL_Nef_polyhedron3 *N = NULL;
	bool plane_error = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		bool err = createPolyhedronFromPolySet(ps, P);
		// if (!err) {
		// 	PRINTB("Polyhedron is closed: %d", P.is_closed());
		// 	PRINTB("Polyhedron is valid: %d", P.is_valid(true, 0));
		// }

		if (!err) N = new CGAL_Nef_polyhedron3(P);
	}
	catch (const CGAL::Assertion_exception &e) {
		if (std::string(e.what()).find("Plane_constructor")!=std::string::npos) {
			if (std::string(e.what()).find("has_on")!=std::string::npos) {
				PRINT("PolySet has nonplanar faces. Attempting alternate construction");
				plane_error=true;
			}
		} else {
			PRINTB("CGAL error in CGAL_Nef_polyhedron3(): %s", e.what());
		}
	}
	if (plane_error) try {
			PolySet ps2(3);
			CGAL_Polyhedron P;
			PolysetUtils::tessellate_faces(ps, ps2);
			bool err = createPolyhedronFromPolySet(ps2,P);
			if (!err) N = new CGAL_Nef_polyhedron3(P);
		}
		catch (const CGAL::Assertion_exception &e) {
			PRINTB("Alternate construction failed. CGAL error in CGAL_Nef_polyhedron3(): %s", e.what());
		}
	CGAL::set_error_behaviour(old_behaviour);
	return new CGAL_Nef_polyhedron(N);
}

static CGAL_Nef_polyhedron *createNefPolyhedronFromPolygon2d(const Polygon2d &polygon)
{
	shared_ptr<PolySet> ps(polygon.tessellate());
	return createNefPolyhedronFromPolySet(*ps);
}

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

#endif /* ENABLE_CGAL */

