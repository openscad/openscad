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
	PolySet ps_tri(3, psq.convexValue());
	PolysetUtils::tessellate_faces(psq, ps_tri);
	if (ps_tri.is_convex()) {
		typedef CGAL::Epick K;
		// Collect point cloud
		// FIXME: Use unordered container (need hash)
		// NB! CGAL's convex_hull_3() doesn't like std::set iterators, so we use a list
		// instead.
		std::list<K::Point_3> points;
		for(const auto &poly : psq.polygons) {
			for(const auto &p : poly) {
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


static void updateCenterOfMass( CGAL_Polyhedron::Point_3 const& p1,
                                CGAL_Polyhedron::Point_3 const& p2,
                                CGAL_Polyhedron::Point_3 const& p3,
                                CGAL_Polyhedron::Point_3 const& ps,
                                NT3 &volumeTotal,
                                NT3 centerOfMass[3]) {
        //std::cout << "p1 ("<<p1.x()<<","<<p1.y()<<","<<p1.z()<<")\n";
        //std::cout << "p2 ("<<p2.x()<<","<<p2.y()<<","<<p2.z()<<")\n";
        //std::cout << "p3 ("<<p3.x()<<","<<p3.y()<<","<<p3.z()<<")\n";
        //std::cout << "ps ("<<ps.x()<<","<<ps.y()<<","<<ps.z()<<")\n";
        NT3 vol=abs(CGAL::volume(ps,p1,p2,p3));
        NT3 centroid[3];
        centroid[0]=(p1[0]+p2[0]+p3[0]+ps[0])/4;
        centroid[1]=(p1[1]+p2[1]+p3[1]+ps[1])/4;
        centroid[2]=(p1[2]+p2[2]+p3[2]+ps[2])/4;
        volumeTotal+=vol;
        centerOfMass[0]+=centroid[0]*vol;
        centerOfMass[1]+=centroid[1]*vol;
        centerOfMass[2]+=centroid[2]*vol;
        //std::cout << "volume="<<to_double(vol)<<" centroid="<<to_double(centroid[0])<<","<<to_double(centroid[1])<<","<<to_double(centroid[2])<<"\n";
        //std::cout<<"center of mass is ("<<to_double(centerOfMass[0])<<","<<to_double(centerOfMass[1])<<","<<to_double(centerOfMass[2])<<")\n";
}



namespace CGALUtils {

	void computeVolume(const PolySet &ps,NT3 &volumeTotal,NT3 centerOfMass[3])
        {
                std::cout << "volume of polyset!"<<std::endl;
                CGAL_Polyhedron poly;
                CGALUtils::createPolyhedronFromPolySet(ps, poly);
                CGAL_Nef_polyhedron *p = createNefPolyhedronFromGeometry(ps);
                if (!p->isEmpty()) computeVolume(*p->p3,volumeTotal,centerOfMass);
        }

        void computeVolume(const CGAL_Nef_polyhedron3 &N,NT3 &volumeTotal,NT3 centerOfMass[3])
        {
                // attention, il faut etre sur que N->p3()->is_simple(), sinon ca fonctionne pas fort
                std::cout << "volumen of nef!"<<std::endl;
                CGAL::Timer t;
                std::vector<CGAL_Polyhedron> parts;
                t.start();
                CGAL_Nef_polyhedron3 decomp=N;
                CGAL::convex_decomposition_3(decomp);
                std::cout <<"nb volumes = "<<decomp.number_of_volumes()<<"\n";
                // the first volume is the outer volume, which ignored in the decomposition
                CGAL_Nef_polyhedron3::Volume_const_iterator ci = decomp.volumes_begin();
                for(; ci != decomp.volumes_end(); ++ci) {
                        std::cout <<"one part...\n";
                        /*CGAL_Nef_polyhedron3::Shell_entry_const_iterator si=ci->shells_begin();
                        for(; si != ci->shells_end(); ++si) {
                                std::cout <<"shell\n";
                                std::cout<<" converting to poly.\n";
                                CGAL_Polyhedron poly;
                                decomp.convert_inner_shell_to_polyhedron(si, poly);
                                parts.push_back(poly);
                        }
                        */

                        if(ci->mark()) {
                                std::cout<<" converting to poly.\n";
                                CGAL_Polyhedron poly;
                                decomp.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
                                parts.push_back(poly);
                        }

                }
                t.stop();
                PRINTB("Center of mass: decomposed into %d convex parts", parts.size());
                PRINTB("Center of mass: decomposition took %f s", t.time());

                volumeTotal=0;
                centerOfMass[0]=0;
                centerOfMass[1]=0;
                centerOfMass[2]=0;
                // on affiche chaque partie...
                for(int i=0;i<(int)parts.size();i++) {
                        std::cout<<"part "<<i<<" : facets = "<<parts[i].size_of_facets()<<", vertex="<<parts[i].size_of_vertices()<<"\n";
                        //
                        // on trouve le centre des points (xs,ys,zs)
                        //
                        CGAL_Polyhedron::Vertex_const_iterator vi = parts[i].vertices_begin();
                        NT3 xs=0,ys=0,zs=0;
                        int nb=0;
                        for(;vi!=parts[i].vertices_end();vi++) {
                                CGAL_Polyhedron::Point_3 const& p = vi->point();
                                xs+=p[0];
                                ys+=p[1];
                                zs+=p[2];
                                nb++;
                                //std::cout << "vertice ("<<x<<","<<y<<","<<z<<")\n";
                        }
                        CGAL_Polyhedron::Point_3 ps(xs/nb,ys/nb,zs/nb);
                        //std::cout << "center is ("<<ps[0]<<","<<ps[1]<<","<<ps[2]<<")\n";
                        //
                        // on visite chaque facette...
                        // on la triangule
                        //
                        CGAL_Polyhedron::Facet_const_iterator fi = parts[i].facets_begin();
                        for(;fi!=parts[i].facets_end();fi++) {
                                //std::cout << "facet "<<fi->is_triangle()<<" , nbedge="<<fi->facet_degree()<<"\n";
                                if( fi->is_triangle() ) {
                                        // tetrahedre avec le centre... et voila!
                                        CGAL_Polyhedron::Facet::Halfedge_around_facet_const_circulator he,end;
                                        end = he = fi->facet_begin();
                                        //CGAL_Polyhedron P;

                                        CGAL_Polyhedron::Point_3 const& p1 = he->vertex()->point();he++;
                                        CGAL_Polyhedron::Point_3 const& p2 = he->vertex()->point();he++;
                                        CGAL_Polyhedron::Point_3 const& p3 = he->vertex()->point();
                                        updateCenterOfMass(p1,p2,p3,ps,volumeTotal,centerOfMass);

                                }else{
                                        // on decoupe!
                                        // trouve le centre de la facette
                                        CGAL_Polyhedron::Facet::Halfedge_around_facet_const_circulator he,end;
                                        NT3 xc=0,yc=0,zc=0;
                                        end = he = fi->facet_begin();
                                        CGAL_For_all(he,end) {
                                                CGAL_Polyhedron::Point_3 const& p = he->vertex()->point();
                                                xc+=p[0];yc+=p[1];zc+=p[2];
                                                //std::cout << "facet halfedge ("<<to_double(p[0])<<","<<to_double(p[1])<<","<<to_double(p[2])<<")\n";
                                        }
                                        xc/=(int)fi->facet_degree();
                                        yc/=(int)fi->facet_degree();
                                        zc/=(int)fi->facet_degree();
                                        CGAL_Polyhedron::Point_3 pc(xc,yc,zc);
                                        //std::cout << "facet center ("<<to_double(xc)<<","<<to_double(yc)<<","<<to_double(zc)<<")\n";
                                        // on fait le tour de la facette, en ajoutant les tetra (p1,p2,ps,pc)
                                        end = he = fi->facet_begin();
                                        CGAL_For_all(he,end) {
                                                CGAL_Polyhedron::Point_3 const& p1 = he->vertex()->point();
                                                CGAL_Polyhedron::Point_3 const& p2 = he->next()->vertex()->point();
                                                //std::cout << "facet halfedge p1 ("<<to_double(p1[0])<<","<<to_double(p1[1])<<","<<to_double(p1[2])<<")\n";
                                                //std::cout << "facet halfedge p2 ("<<to_double(p2[0])<<","<<to_double(p2[1])<<","<<to_double(p2[2])<<")\n";
                                                updateCenterOfMass(p1,p2,pc,ps,volumeTotal,centerOfMass);
                                        }
                                }
                        }
                }
                // finalise le centre de masse
                if( volumeTotal>0 ) {
                        centerOfMass[0]/=volumeTotal;
                        centerOfMass[1]/=volumeTotal;
                        centerOfMass[2]/=volumeTotal;
                }
                std::cout<<"Volume Total = "<<to_double(volumeTotal)<<"\n";
                std::cout<<"center of mass is ("<<to_double(centerOfMass[0])<<","<<to_double(centerOfMass[1])<<","<<to_double(centerOfMass[2])<<")\n";
        //      std::cout<<"center of mass is ("<<centerOfMass[0]<<","<<centerOfMass[1]<<","<<centerOfMass[2]<<")\n";

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
		std::vector<std::vector<IndexedFace>> polygons;

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
		for(const auto &faces : polygons) {
#if 0 // For debugging
			std::cerr << "---\n";
			for(const auto &poly : faces) {
				for(auto i : poly) {
					std::cerr << i << " ";
				}
				std::cerr << "\n";
			}
#if 0 // debug
			std::cerr.precision(20);
			for(const auto &poly : faces) {
				for(auto i : poly) {
					std::cerr << verts[i][0] << "," << verts[i][1] << "," << verts[i][2] << "\n";
				}
				std::cerr << "\n";
			}
#endif // debug
			std::cerr << "-\n";
#endif // debug
#if 0 // For debugging
		std::cerr.precision(20);
		for (size_t i=0;i<allVertices.size();i++) {
			std::cerr << verts[i][0] << ", " << verts[i][1] << ", " << verts[i][2] << "\n";
		}		
#endif // debug

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
				for(const auto &t : triangles) {
					assert(t[0] >= 0 && t[0] < (int)allVertices.size());
					assert(t[1] >= 0 && t[1] < (int)allVertices.size());
					assert(t[2] >= 0 && t[2] < (int)allVertices.size());
					allTriangles.push_back(t);
				}
			}
		}

#if 0 // For debugging
		for(const auto &t : allTriangles) {
			std::cerr << t[0] << " " << t[1] << " " << t[2] << "\n";
		}
#endif // debug
		// 4. Validate mesh (manifoldness)
		int unconnected2 = GeometryUtils::findUnconnectedEdges(allTriangles);
		if (unconnected2 > 0) {
			PRINTB("Error: Non-manifold triangle mesh created: %d unconnected edges", unconnected2);
		}

		for(const auto &t : allTriangles) {
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
#endif // debug

		return err;
	}
#endif // createPolySetFromNefPolyhedron3
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
				for(const auto &p : triangles) {
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
#endif // createPolySetFromNefPolyhedron3
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
				for(const auto &p : triangles) {
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
#endif // createPolySetFromNefPolyhedron3
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
				for (size_t i=0;i<indices.size();i++) {
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
				for(const auto &p : triangles) {
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
#endif // createPolySetFromNefPolyhedron3
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
			for(const auto &poly : polyholes) {
				for(const auto &v : poly) {
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
				for(const auto &p : triangles) {
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
#endif // createPolySetFromNefPolyhedron3
#if 0
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
			for(const auto &poly : polyhole.faces) {
				for(auto i : poly) {
					std::cerr << polyhole.vertices[i][0] << "," << polyhole.vertices[i][1] << "," << polyhole.vertices[i][2] << "\n";
				}
				std::cerr << "\n";
			}
			std::cerr << "-\n";
#endif // debug

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
				for(const auto &t : triangles) {
					ps.append_poly();
					ps.append_vertex(verts[t[0]]);
					ps.append_vertex(verts[t[1]]);
					ps.append_vertex(verts[t[2]]);
				}
			}
		}
	}
#endif // createPolySetFromNefPolyhedron3


}; // namespace CGALUtils

#endif /* ENABLE_CGAL */







