#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "Polygon2d.h"
#include "polyset-utils.h"
#include "grid.h"

#include "cgal.h"
#include <CGAL/convex_hull_3.h>

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
				if (target.isEmpty()) target = src.copy();
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

	CGAL_Nef_polyhedron project(const CGAL_Nef_polyhedron &N, bool cut)
	{
		logstream log(5);
		CGAL_Nef_polyhedron nef_poly(2);
		if (N.getDimension() != 3) return nef_poly;

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
				return nef_poly;
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
				nef_poly.p2 = zremover.output_nefpoly2d;
			}	catch (const CGAL::Failure_exception &e) {
				PRINTB("CGAL error in CGALUtils::project while flattening: %s", e.what());
			}
			log << "</svg>\n";
				
			CGAL::set_error_behaviour(old_behaviour);
		}
		// In projection mode all the triangles are projected manually into the XY plane
		else {
			PolySet *ps3 = N.convertToPolyset();
			if (!ps3) return nef_poly;
			const Polygon2d *poly = PolysetUtils::project(*ps3);

			// FIXME: Convert back to Nef2 and delete poly?
/*			if (nef_poly.isEmpty()) {
				nef_poly.p2.reset(new CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED));
				}
				else {
				(*nef_poly.p2) += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
				}
*/
			delete ps3;
		}
		return nef_poly;
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

class CGAL_Build_PolySet : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	typedef CGAL_HDS::Vertex::Point CGALPoint;

	const PolySet &ps;
	CGAL_Build_PolySet(const PolySet &ps) : ps(ps) { }

	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);
		typedef boost::tuple<double, double, double> BuilderVertex;
		typedef std::map<BuilderVertex, size_t> BuilderMap;
		BuilderMap vertices;
		std::vector<size_t> indices(3);

		// Estimating same # of vertices as polygons (very rough)
		B.begin_surface(ps.polygons.size(), ps.polygons.size());
		int pidx = 0;
		printf("polyhedron(triangles=[");
		BOOST_FOREACH(const PolySet::Polygon &p, ps.polygons) {
			if (pidx++ > 0) printf(",");
			indices.clear();
			BOOST_FOREACH(const Vector3d &v, p) {
				size_t idx;
				BuilderVertex bv = boost::make_tuple(v[0], v[1], v[2]);
        if (vertices.count(bv) > 0) indices.push_back(vertices[bv]);
				else {
					indices.push_back(vertices.size());
					vertices[bv] = vertices.size();
					B.add_vertex(CGALPoint(v[0], v[1], v[2]));
				}
			}
			B.begin_facet();
			printf("[");
			int fidx = 0;
			BOOST_FOREACH(size_t i, indices) {
				B.add_vertex_to_facet(i);
				if (fidx++ > 0) printf(",");
				printf("%ld", i);
			}
			printf("]");
			B.end_facet();
		}
		B.end_surface();
		printf("],\n");

		printf("points=[");
		int vidx = 0;
		for (int vidx=0;vidx<vertices.size();vidx++) {
			if (vidx > 0) printf(",");
			const BuilderMap::const_iterator it = 
				std::find_if(vertices.begin(), vertices.end(), 
										 boost::bind(&BuilderMap::value_type::second, _1) == vidx);
			printf("[%g,%g,%g]", it->first.get<0>(), it->first.get<1>(), it->first.get<2>());
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

CGAL_Iso_cuboid_3 bounding_box( const CGAL_Nef_polyhedron3 &N )
{
	CGAL_Iso_cuboid_3 result(0,0,0,0,0,0);
	CGAL_Nef_polyhedron3::Vertex_const_iterator vi;
	std::vector<CGAL_Nef_polyhedron3::Point_3> points;
	// can be optimized by rewriting bounding_box to accept vertices
	CGAL_forall_vertices( vi, N )
		points.push_back( vi->point() );
	if (points.size())
		result = CGAL::bounding_box( points.begin(), points.end() );
	return result;
}

CGAL_Iso_rectangle_2e bounding_box( const CGAL_Nef_polyhedron2 &N )
{
	CGAL_Iso_rectangle_2e result(0,0,0,0);
	CGAL_Nef_polyhedron2::Explorer explorer = N.explorer();
	CGAL_Nef_polyhedron2::Explorer::Vertex_const_iterator vi;
	std::vector<CGAL_Point_2e> points;
	// can be optimized by rewriting bounding_box to accept vertices
	for ( vi = explorer.vertices_begin(); vi != explorer.vertices_end(); ++vi )
		if ( explorer.is_standard( vi ) )
			points.push_back( explorer.point( vi ) );
	if (points.size())
		result = CGAL::bounding_box( points.begin(), points.end() );
	return result;
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
	if (ps.empty()) return new CGAL_Nef_polyhedron(ps.is2d ? 2 : 3);

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
		return new CGAL_Nef_polyhedron(N);
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

		return new CGAL_Nef_polyhedron(N);
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
		return new CGAL_Nef_polyhedron(pr.toNef());
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

		return new CGAL_Nef_polyhedron(N);

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
			PRINTB("CGAL error in CGALUtils::createNefPolyhedronFromPolySet(): %s", e.what());
		}
		CGAL::set_error_behaviour(old_behaviour);
		return new CGAL_Nef_polyhedron(N);
	}
	return NULL;
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
	assert(false && "CGALEvaluator::evaluateCGALMesh(): Unsupported geometry type");
	return NULL;
}

#endif /* ENABLE_CGAL */

