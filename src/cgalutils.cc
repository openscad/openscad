// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "Polygon2d.h"
#include "grid.h"
#include "node.h"
#include "degree_trig.h"

#include "cgal.h"
#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>

#include <CGAL/config.h> 
#include <CGAL/version.h> 

#include <CGAL/convex_hull_3.h>
#pragma pop_macro("NDEBUG")

#include "svg.h"
#include "hash.h"
#include "GeometryUtils.h"

#include <map>
#include <queue>

static CGAL_Nef_polyhedron *createNefPolyhedronFromPolySet(const PolySet &ps)
{
	if (ps.isEmpty()) return new CGAL_Nef_polyhedron();
	assert(ps.getDimension() == 3);

	// Since is_convex doesn't work well with non-planar faces,
	// we tessellate the polyset before checking.
	PolySet psq(ps);
	psq.quantizeVertices();
	PolySet ps_tri(psq);
	ps_tri.tessellate();
	if (ps_tri.isConvex()) {
		typedef CGAL::Epick K;
		// Collect point cloud
		std::vector<K::Point_3> points;
		psq.getVertices<K::Point_3,std::vector<K::Point_3>>(points, vector_convert<K::Point_3,Vector3d>);
		
		if (points.size() <= 3) return new CGAL_Nef_polyhedron();

		// Apply hull
		CGAL::Polyhedron_3<K> r;
		CGAL::convex_hull_3(points.begin(), points.end(), r);
		CGAL_Polyhedron r_exact;
		CGALUtils::copyPolyhedron(r, r_exact);
		return new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(r_exact));
	}

	CGAL_Nef_polyhedron3 *N = nullptr;
	auto plane_error = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Polyhedron P;
		auto err = CGALUtils::createPolyhedronFromPolySet(psq, P);
		 if (!err) {
		 	PRINTDB("Polyhedron is closed: %d", P.is_closed());
		 	PRINTDB("Polyhedron is valid: %d", P.is_valid(false, 0));
		 }

		if (!err) N = new CGAL_Nef_polyhedron3(P);
	}
	catch (const CGAL::Assertion_exception &e) {
		// First two tests matches against CGAL < 4.10, the last two tests matches against CGAL >= 4.10
		if ((std::string(e.what()).find("Plane_constructor") != std::string::npos &&
				 std::string(e.what()).find("has_on") != std::string::npos) ||
				std::string(e.what()).find("ss_plane.has_on(sv_prev->point())") != std::string::npos ||
				std::string(e.what()).find("ss_circle.has_on(sp)") != std::string::npos) {
			LOG(message_group::None,Location::NONE,"","PolySet has nonplanar faces. Attempting alternate construction");
			plane_error=true;
		} else {
			LOG(message_group::Error,Location::NONE,"","CGAL error in CGAL_Nef_polyhedron3(): %1$s",e.what());

		}
	}
	if (plane_error) try {
			CGAL_Polyhedron P;
			auto err = CGALUtils::createPolyhedronFromPolySet(ps_tri, P);
            if (!err) {
                PRINTDB("Polyhedron is closed: %d", P.is_closed());
                PRINTDB("Polyhedron is valid: %d", P.is_valid(false, 0));
            }
			if (!err) N = new CGAL_Nef_polyhedron3(P);
		}
		catch (const CGAL::Assertion_exception &e) {
			LOG(message_group::Error,Location::NONE,"","Alternate construction failed. CGAL error in CGAL_Nef_polyhedron3(): %1$s",e.what());
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

	CGAL_Iso_cuboid_3 boundingBox(const CGAL_Nef_polyhedron3 &N)
	{
		CGAL_Iso_cuboid_3 result(0,0,0,0,0,0);
		CGAL_Nef_polyhedron3::Vertex_const_iterator vi;
		std::vector<CGAL_Nef_polyhedron3::Point_3> points;
		// can be optimized by rewriting bounding_box to accept vertices
		CGAL_forall_vertices(vi, N) points.push_back(vi->point());
		if (points.size()) result = CGAL::bounding_box(points.begin(), points.end());
		return result;
	}

	namespace {

		// lexicographic comparison
		bool operator < (Vector3d const& a, Vector3d const& b) {
			for (int i = 0; i < 3; ++i) {
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

		const double angle_threshold = cos_degrees(.1); // .1°

		typedef CGAL::Simple_cartesian<double> K;
		typedef K::Vector_3 Vector;
		typedef K::Point_3 Point;
		typedef K::Plane_3 Plane;

		// compute edge to face relations and plane equations
		typedef std::pair<Vector3d,Vector3d> Edge;
		typedef std::map<Edge, int, VecPairCompare> Edge_to_facet_map;
		Edge_to_facet_map edge_to_facet_map;
		std::vector<Plane> facet_planes;
		facet_planes.reserve(ps.getPolygons().size());

		for (size_t i = 0; i < ps.getPolygons().size(); ++i) {
			Plane plane;
			auto N = ps.getPolygons()[i].size();
			if (N >= 3) {
				std::vector<Point> v(N);
				for (size_t j = 0; j < N; ++j) {
					v[j] = vector_convert<Point>(ps.getPolygons()[i][j]);
					Edge edge(ps.getPolygons()[i][j],ps.getPolygons()[i][(j+1)%N]);
					if (edge_to_facet_map.count(edge)) return false; // edge already exists: nonmanifold
					edge_to_facet_map[edge] = i;
				}
				Vector normal;
				CGAL::normal_vector_newell_3(v.begin(), v.end(), normal);
				plane = Plane(v[0], normal);
			}
			facet_planes.push_back(plane);
		}

		for (size_t i = 0; i < ps.getPolygons().size(); ++i) {
			auto N = ps.getPolygons()[i].size();
			if (N < 3) continue;
			for (size_t j = 0; j < N; ++j) {
				Edge other_edge(ps.getPolygons()[i][(j+1)%N], ps.getPolygons()[i][j]);
				if (edge_to_facet_map.count(other_edge) == 0) return false;//
				//Edge_to_facet_map::const_iterator it = edge_to_facet_map.find(other_edge);
				//if (it == edge_to_facet_map.end()) return false; // not a closed manifold
				//int other_facet = it->second;
				int other_facet = edge_to_facet_map[other_edge];

				auto p = vector_convert<Point>(ps.getPolygons()[i][(j+2)%N]);

				if (facet_planes[other_facet].has_on_positive_side(p)) {
					// Check angle
					const auto& u = facet_planes[other_facet].orthogonal_vector();
					const auto& v = facet_planes[i].orthogonal_vector();

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

			for (size_t i = 0; i < ps.getPolygons()[f].size(); ++i) {
				int j = (i+1) % ps.getPolygons()[f].size();
				auto it = edge_to_facet_map.find(Edge(ps.getPolygons()[f][j], ps.getPolygons()[f][i]));
				if (it == edge_to_facet_map.end()) return false; // Nonmanifold
				if (!explored_facets.count(it->second)) {
					explored_facets.insert(it->second);
					facets_to_visit.push(it->second);
				}
			}
		}

		// Make sure that we were able to reach all polygons during our visit
		return explored_facets.size() == ps.getPolygons().size();
	}


	CGAL_Nef_polyhedron *createNefPolyhedronFromGeometry(const Geometry &geom)
	{
		if (auto ps = dynamic_cast<const PolySet*>(&geom)) {
			return createNefPolyhedronFromPolySet(*ps);
		}
		else if (auto poly2d = dynamic_cast<const Polygon2d*>(&geom)) {
			return createNefPolyhedronFromPolygon2d(*poly2d);
		}
		assert(false && "createNefPolyhedronFromGeometry(): Unsupported geometry type");
		return nullptr;
	}

/*
	Create a PolySet from a Nef Polyhedron 3. return false on success, 
	true on failure. The trick to this is that Nef Polyhedron3 faces have 
	'holes' in them. . . while PolySet (and many other 3d polyhedron 
	formats) do not allow for holes in their faces. The function documents 
	the method used to deal with this
*/
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		// 1. Build Indexed PolyMesh
		// 2. Validate mesh (manifoldness)
		// 3. Triangulate each face
		//    -> IndexedTriangleMesh
		// 4. Validate mesh (manifoldness)
		// 5. Create PolySet

		// 1. Build Indexed PolyMesh
		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			// Since we're downscaling to float, vertices might merge during this conversion.
			// To avoid passing equal vertices to the tessellator, we remove consecutively identical
			// vertices.
			ps.append_poly_only();
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (!hfaceti->incident_volume()->mark()) {
				CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
				CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
					CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
					CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
					ps.append_facet();
					CGAL_For_all(c1, c2) {
						CGAL_Point_3 p = c1->source()->center_vertex()->point();
						ps.append_vertex(vector_convert<Vector3d>(p));
					}
					ps.close_facet();
				}
			}
			ps.close_poly_only();
		}

		// 2. Validate mesh (manifoldness)
		auto unconnected = GeometryUtils::findUnconnectedEdges(ps.getIndexedPolygons());
		if (unconnected > 0) {
			LOG(message_group::Error,Location::NONE,"","Non-manifold mesh encountered: %1$d unconnected edges",unconnected);
		}
		
		// 3. Triangulate each face
		ps.tessellate();

		// 4. Validate mesh (manifoldness)
		auto unconnected2 = GeometryUtils::findUnconnectedEdges(ps.getIndexedTriangles());
		if (unconnected2 > 0) {
			LOG(message_group::Error,Location::NONE,"","Non-manifold mesh created: %1$d unconnected edges",unconnected2);
		}

		return false;
	}
}; // namespace CGALUtils

#endif /* ENABLE_CGAL */







