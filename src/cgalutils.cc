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
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>

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
		BOOST_FOREACH(const Polygon &poly, psq.polygons) {
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

		for (size_t i = 0; i < ps.polygons.size(); i++) {
			Plane plane;
			size_t N = ps.polygons[i].size();
			if (N >= 3) {
				std::vector<Point> v(N);
				for (size_t j = 0; j < N; j++) {
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

		for (size_t i = 0; i < ps.polygons.size(); i++) {
			size_t N = ps.polygons[i].size();
			if (N < 3) continue;
			for (size_t j = 0; j < N; j++) {
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

			for (size_t i = 0; i < ps.polygons[f].size(); i++) {
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
#if 1
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps)
	{
		// 1. Build Indexed PolyMesh
		// 2. Validate mesh (manifoldness)
		// 3. Triangulate each face
		//    -> IndexedTriangleMesh
		// 4. Validate mesh (manifoldness)
		// 5. Create PolySet

		bool err = false;

		// 1. Build Indexed PolyMesh
		Reindexer<Vector3f> allVertices;
		std::vector<std::vector<IndexedFace> > polygons;

		CGAL_Nef_polyhedron3::Halffacet_const_iterator hfaceti;
		CGAL_forall_halffacets(hfaceti, N) {
			CGAL::Plane_3<CGAL_Kernel3> plane(hfaceti->plane());
			// Since we're downscaling to float, vertices might merge during this conversion.
			// To avoid passing equal vertices to the tessellator, we remove consecutively identical
			// vertices.
			polygons.push_back(std::vector<IndexedFace>());
			std::vector<IndexedFace> &faces = polygons.back();
			// the 0-mark-volume is the 'empty' volume of space. skip it.
			if (!hfaceti->incident_volume()->mark()) {
				CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator cyclei;
				CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
					CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(cyclei);
					CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c2(c1);
					faces.push_back(IndexedFace());
					IndexedFace &currface = faces.back();
					CGAL_For_all(c1, c2) {
						CGAL_Point_3 p = c1->source()->center_vertex()->point();
						// Create vertex indices and remove consecutive duplicate vertices
						int idx = allVertices.lookup(vector_convert<Vector3f>(p));
						if (currface.empty() || idx != currface.back()) currface.push_back(idx);
					}
					if (!currface.empty() && currface.front() == currface.back()) currface.pop_back();
					if (currface.size() < 3) faces.pop_back(); // Cull empty triangles
				}
			}
			if (faces.empty()) polygons.pop_back(); // Cull empty faces
		}

		// 2. Validate mesh (manifoldness)
		int unconnected = GeometryUtils::findUnconnectedEdges(polygons);
		if (unconnected > 0) {
			PRINTB("Error: Non-manifold mesh encountered: %d unconnected edges", unconnected);
		}
		// 3. Triangulate each face
		const Vector3f *verts = allVertices.getArray();
		std::vector<IndexedTriangle> allTriangles;
		BOOST_FOREACH(const std::vector<IndexedFace> &faces, polygons) {
#if 0 // For debugging
			std::cerr << "---\n";
			BOOST_FOREACH(const IndexedFace &poly, faces) {
				BOOST_FOREACH(int i, poly) {
					std::cerr << i << " ";
				}
				std::cerr << "\n";
			}
#if 0
			std::cerr.precision(20);
			BOOST_FOREACH(const IndexedFace &poly, faces) {
				BOOST_FOREACH(int i, poly) {
					std::cerr << verts[i][0] << "," << verts[i][1] << "," << verts[i][2] << "\n";
				}
				std::cerr << "\n";
			}
#endif
			std::cerr << "-\n";
#endif
#if 0 // For debugging
		std::cerr.precision(20);
		for (size_t i=0;i<allVertices.size();i++) {
			std::cerr << verts[i][0] << ", " << verts[i][1] << ", " << verts[i][2] << "\n";
		}		
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
			bool err = GeometryUtils::tessellatePolygonWithHoles(verts, faces, triangles, NULL);
			if (!err) {
				BOOST_FOREACH(const IndexedTriangle &t, triangles) {
					assert(t[0] >= 0 && t[0] < (int)allVertices.size());
					assert(t[1] >= 0 && t[1] < (int)allVertices.size());
					assert(t[2] >= 0 && t[2] < (int)allVertices.size());
					allTriangles.push_back(t);
				}
			}
		}

#if 0 // For debugging
		BOOST_FOREACH(const IndexedTriangle &t, allTriangles) {
			std::cerr << t[0] << " " << t[1] << " " << t[2] << "\n";
		}
#endif
		// 4. Validate mesh (manifoldness)
		int unconnected2 = GeometryUtils::findUnconnectedEdges(allTriangles);
		if (unconnected2 > 0) {
			PRINTB("Error: Non-manifold triangle mesh created: %d unconnected edges", unconnected2);
		}

		BOOST_FOREACH(const IndexedTriangle &t, allTriangles) {
			ps.append_poly();
			ps.append_vertex(verts[t[0]]);
			ps.append_vertex(verts[t[1]]);
			ps.append_vertex(verts[t[2]]);
		}

#if 0 // For debugging
		std::cerr.precision(20);
		for (size_t i=0;i<allVertices.size();i++) {
			std::cerr << verts[i][0] << ", " << verts[i][1] << ", " << verts[i][2] << "\n";
		}		
#endif

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

#endif /* ENABLE_CGAL */


