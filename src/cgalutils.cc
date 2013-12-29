#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "Polygon2d.h"
#include "polyset-utils.h"
#include "grid.h"

#include "cgal.h"
#include <CGAL/convex_hull_3.h>
#include "svg.h"
#include "Reindexer.h"

#include <map>
#include <boost/foreach.hpp>

namespace CGALUtils {

	bool applyHull(const Geometry::ChildList &children, CGAL_Polyhedron &result)
	{
		// Collect point cloud
		std::list<CGAL_Polyhedron::Vertex::Point_3> points;
		CGAL_Polyhedron P;
		BOOST_FOREACH(const Geometry::ChildItem &item, children) {
			const shared_ptr<const Geometry> &chgeom = item.second;
			const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(chgeom.get());
			if (N) {
				if (!N->p3->is_simple()) {
					PRINT("Hull() currently requires a valid 2-manifold. Please modify your design. See http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export");
				}
				else {
					bool err = true;
					std::string errmsg("");
					try {
						err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>( *(N->p3), P );
						// N->p3->convert_to_Polyhedron(P);
					}
					catch (const CGAL::Failure_exception &e) {
						err = true;
						errmsg = std::string(e.what());
					}
					if (err) {
						PRINT("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed.");
						if (errmsg!="") PRINTB("ERROR: %s",errmsg);
					} else {
						std::transform(P.vertices_begin(), P.vertices_end(), std::back_inserter(points), 
													 boost::bind(static_cast<const CGAL_Polyhedron::Vertex::Point_3&(CGAL_Polyhedron::Vertex::*)() const>(&CGAL_Polyhedron::Vertex::point), _1));
					}
				}
			}
			else {
				const PolySet *ps = dynamic_cast<const PolySet *>(chgeom.get());
				BOOST_FOREACH(const PolySet::Polygon &p, ps->polygons) {
					BOOST_FOREACH(const Vector3d &v, p) {
						points.push_back(CGAL_Polyhedron::Vertex::Point_3(v[0], v[1], v[2]));
					}
				}
			}
		}
		if (points.size() > 0) {
			// Apply hull
			if (points.size() > 3) {
				CGAL::convex_hull_3(points.begin(), points.end(), result);
				return true;
			}
		}
		return false;
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
		if (src.isEmpty()) return; // Empty polyhedron. This can happen for e.g. square([0,0])
		if (target.isEmpty() && op != OPENSCAD_UNION) return; // empty op <something> => empty
		if (target.getDimension() != src.getDimension()) return; // If someone tries to e.g. union 2d and 3d objects

		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			switch (op) {
			case OPENSCAD_UNION:
				if (target.isEmpty()) target = *src.copy();
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

	static Polygon2d *convertToPolygon2d(const CGAL_Nef_polyhedron2 &p2)
	{
		Polygon2d *poly = new Polygon2d;
		
		typedef CGAL_Nef_polyhedron2::Explorer Explorer;
		typedef Explorer::Face_const_iterator fci_t;
		typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
		Explorer E = p2.explorer();
		
		for (fci_t fit = E.faces_begin(), facesend = E.faces_end(); fit != facesend; ++fit)	{
			heafcc_t fcirc(E.halfedge(fit)), fend(fcirc);
			Outline2d outline;
			CGAL_For_all(fcirc, fend) {
				if (E.is_standard(E.target(fcirc))) {
					Explorer::Point ep = E.point(E.target(fcirc));
					outline.vertices.push_back(Vector2d(to_double(ep.x()),
																		 to_double(ep.y())));
				}
			}
			if (outline.vertices.size() > 0) poly->addOutline(outline);
		}
		return poly;
	}

	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut)
	{
		logstream log(5);
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
				
			log << OpenSCAD::svg_header( 480, 100000 ) << "\n";
			try {
				ZRemover zremover;
				CGAL_Nef_polyhedron3::Volume_const_iterator i;
				CGAL_Nef_polyhedron3::Shell_entry_const_iterator j;
				CGAL_Nef_polyhedron3::SFace_const_handle sface_handle;
				for ( i = newN.p3->volumes_begin(); i != newN.p3->volumes_end(); ++i ) {
					log << "<!-- volume. mark: " << i->mark() << " -->\n";
					for ( j = i->shells_begin(); j != i->shells_end(); ++j ) {
						log << "<!-- shell. mark: " << i->mark() << " -->\n";
						sface_handle = CGAL_Nef_polyhedron3::SFace_const_handle( j );
						newN.p3->visit_shell_objects( sface_handle , zremover );
						log << "<!-- shell. end. -->\n";
					}
					log << "<!-- volume end. -->\n";
				}
				poly = convertToPolygon2d(*zremover.output_nefpoly2d);
			}	catch (const CGAL::Failure_exception &e) {
				PRINTB("CGAL error in CGALUtils::project while flattening: %s", e.what());
			}
			log << "</svg>\n";
				
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

};

bool createPolySetFromPolyhedron(const CGAL_Polyhedron &p, PolySet &ps)
{
	bool err = false;
	typedef CGAL_Polyhedron::Vertex                                 Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator                  VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator                   FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;
		
	for (FCI fi = p.facets_begin(); fi != p.facets_end(); ++fi) {
		HFCC hc = fi->facet_begin();
		HFCC hc_end = hc;
		Vertex v1, v2, v3;
		v1 = *VCI((hc++)->vertex());
		v3 = *VCI((hc++)->vertex());
		do {
			v2 = v3;
			v3 = *VCI((hc++)->vertex());
			double x1 = CGAL::to_double(v1.point().x());
			double y1 = CGAL::to_double(v1.point().y());
			double z1 = CGAL::to_double(v1.point().z());
			double x2 = CGAL::to_double(v2.point().x());
			double y2 = CGAL::to_double(v2.point().y());
			double z2 = CGAL::to_double(v2.point().z());
			double x3 = CGAL::to_double(v3.point().x());
			double y3 = CGAL::to_double(v3.point().y());
			double z3 = CGAL::to_double(v3.point().z());
			ps.append_poly();
			ps.append_vertex(x1, y1, z1);
			ps.append_vertex(x2, y2, z2);
			ps.append_vertex(x3, y3, z3);
		} while (hc != hc_end);
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

	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);
		typedef boost::tuple<double, double, double> BuilderVertex;
		Reindexer<Vector3d> vertices;
		std::vector<size_t> indices(3);

		// Estimating same # of vertices as polygons (very rough)
		B.begin_surface(ps.polygons.size(), ps.polygons.size());
		int pidx = 0;
		printf("polyhedron(faces=[");
		BOOST_FOREACH(const PolySet::Polygon &p, ps.polygons) {
			if (pidx++ > 0) printf(",");
			indices.clear();
			BOOST_FOREACH(const Vector3d &v, p) {
				size_t s = vertices.size();
				size_t idx = vertices.lookup(v);
				// If we added a vertex, also add it to the CGAL builder
				if (idx == s) B.add_vertex(CGALPoint(v[0], v[1], v[2]));
				indices.push_back(idx);
			}
			std::map<size_t,int> fc;
			bool facet_is_degenerate = false;
			BOOST_REVERSE_FOREACH(size_t i, indices) {
				if (fc[i]++ > 0) facet_is_degenerate = true;
			}
			if (!facet_is_degenerate) {
				B.begin_facet();
				printf("[");
				int fidx = 0;
				std::map<int,int> fc;
				BOOST_REVERSE_FOREACH(size_t i, indices) {
					B.add_vertex_to_facet(i);
					if (fidx++ > 0) printf(",");
					printf("%ld", i);
				}
				printf("]");
				B.end_facet();
			}
		}
		B.end_surface();
		printf("],\n");

		printf("points=[");
		for (int vidx=0;vidx<vertices.size();vidx++) {
			if (vidx > 0) printf(",");
			const Vector3d &v = vertices.getArray()[vidx];
			printf("[%g,%g,%g]", v[0], v[1], v[2]);
		}
		printf("]);\n");
	}
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
	log << " <!-- ZRemover Halffacet visit. Mark: " << hfacet->mark() << " -->\n";
	if ( hfacet->plane().orthogonal_direction() != this->up ) {
		log << "  <!-- ZRemover down-facing half-facet. skipping -->\n";
		log << " <!-- ZRemover Halffacet visit end-->\n";
		return;
	}

	// possible optimization - throw out facets that are vertically oriented

	CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator fci;
	int contour_counter = 0;
	CGAL_forall_facet_cycles_of( fci, hfacet ) {
		if ( fci.is_shalfedge() ) {
			log << " <!-- ZRemover Halffacet cycle begin -->\n";
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(fci), cend(c1);
			std::vector<CGAL_Nef_polyhedron2::Explorer::Point> contour;
			CGAL_For_all( c1, cend ) {
				CGAL_Nef_polyhedron3::Point_3 point3d = c1->source()->target()->point();
				CGAL_Nef_polyhedron2::Explorer::Point point2d(CGAL::to_double(point3d.x()),
																											CGAL::to_double(point3d.y()));
				contour.push_back( point2d );
			}
			if (contour.size()==0) continue;

			log << " <!-- is_simple_2:" << CGAL::is_simple_2( contour.begin(), contour.end() ) << " --> \n";

			tmpnef2d.reset( new CGAL_Nef_polyhedron2( contour.begin(), contour.end(), boundary ) );

			if ( contour_counter == 0 ) {
				log << " <!-- contour is a body. make union(). " << contour.size() << " points. -->\n" ;
				*(output_nefpoly2d) += *(tmpnef2d);
			} else {
				log << " <!-- contour is a hole. make intersection(). " << contour.size() << " points. -->\n";
				*(output_nefpoly2d) *= *(tmpnef2d);
			}

			/*log << "\n<!-- ======== output tmp nef: ==== -->\n"
				<< OpenSCAD::dump_svg( *tmpnef2d ) << "\n"
				<< "\n<!-- ======== output accumulator: ==== -->\n"
				<< OpenSCAD::dump_svg( *output_nefpoly2d ) << "\n";*/

			contour_counter++;
		} else {
			log << " <!-- ZRemover trivial facet cycle skipped -->\n";
		}
		log << " <!-- ZRemover Halffacet cycle end -->\n";
	}
	log << " <!-- ZRemover Halffacet visit end -->\n";
}

static CGAL_Nef_polyhedron *createNefPolyhedronFromPolySet(const PolySet &ps)
{
	assert(ps.getDimension() == 3);
	if (ps.isEmpty()) return new CGAL_Nef_polyhedron();

	CGAL_Nef_polyhedron3 *N = NULL;
	bool plane_error = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		bool err = createPolyhedronFromPolySet(ps, P);
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

