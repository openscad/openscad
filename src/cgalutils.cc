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
typedef typename CGAL::Triangulation_vertex_base_2<Kernel> Vb;
//typedef typename CGAL::Constrained_triangulation_face_base_2<Kernel> Fb;
typedef CGAL::Delaunay_mesh_face_base_2<Kernel> Fb;
typedef typename CGAL::Triangulation_data_structure_2<Vb,Fb> TDS;
typedef CGAL::Exact_intersections_tag ITAG;
typedef typename CGAL::Constrained_Delaunay_triangulation_2<Kernel,TDS,ITAG> CDT;
//typedef typename CGAL::Constrained_Delaunay_triangulation_2<Kernel,TDS> CDT;

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
	PRINTB("seeding %i holes",numholes);
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
		PRINTB("seed %f,%f",x%y);
	}
	PRINT("seeding done");

	PRINT( "meshing" );
	CGAL::refine_Delaunay_mesh_2_without_edge_refinement( cdt,
		list_of_seeds.begin(), list_of_seeds.end(),
		DummyCriteria<CDT>() );

	PRINT("meshing done");
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

	PRINTB("built %i triangles\n",triangles.size());
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

