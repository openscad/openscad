#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "cgal.h"
#include <CGAL/bounding_box.h>
typedef CGAL::Point_3<CGAL_Kernel3> CGAL_Point_3;
typedef CGAL_Kernel3::Iso_cuboid_3 CGAL_Iso_cuboid_3;

typedef CGAL_Nef_polyhedron2::Explorer::Point CGAL_Point_2;
// Note- Explorer::Point is incompatible with CGAL::Point_2<CGAL_Kernel2>
typedef CGAL_Kernel2::Iso_rectangle_2 CGAL_Iso_rectangle_2;

#include <boost/algorithm/string.hpp>
#include <map>

PolySet *createPolySetFromPolyhedron(const CGAL_Polyhedron &p)
{
	PolySet *ps = new PolySet();
		
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
			ps->append_poly();
			ps->append_vertex(x1, y1, z1);
			ps->append_vertex(x2, y2, z2);
			ps->append_vertex(x3, y3, z3);
		} while (hc != hc_end);
	}
	return ps;
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

		std::vector<CGALPoint> vertices;
		Grid3d<int> vertices_idx(GRID_FINE);

		for (size_t i = 0; i < ps.polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps.polygons[i];
			for (size_t j = 0; j < poly->size(); j++) {
				const Vector3d &p = poly->at(j);
				if (!vertices_idx.has(p[0], p[1], p[2])) {
					vertices_idx.data(p[0], p[1], p[2]) = vertices.size();
					vertices.push_back(CGALPoint(p[0], p[1], p[2]));
				}
			}
		}

		B.begin_surface(vertices.size(), ps.polygons.size());
#ifdef GEN_SURFACE_DEBUG
		printf("=== CGAL Surface ===\n");
#endif

		for (size_t i = 0; i < vertices.size(); i++) {
			const CGALPoint &p = vertices[i];
			B.add_vertex(p);
#ifdef GEN_SURFACE_DEBUG
			printf("%d: %f %f %f\n", i, p.x().to_double(), p.y().to_double(), p.z().to_double());
#endif
		}

		for (size_t i = 0; i < ps.polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps.polygons[i];
			std::map<int,int> fc;
			bool facet_is_degenerated = false;
			for (size_t j = 0; j < poly->size(); j++) {
				const Vector3d &p = poly->at(j);
				int v = vertices_idx.data(p[0], p[1], p[2]);
				if (fc[v]++ > 0)
					facet_is_degenerated = true;
			}
			
			if (!facet_is_degenerated)
				B.begin_facet();
#ifdef GEN_SURFACE_DEBUG
			printf("F:");
#endif
			for (size_t j = 0; j < poly->size(); j++) {
				const Vector3d &p = poly->at(j);
#ifdef GEN_SURFACE_DEBUG
				printf(" %d (%f,%f,%f)", vertices_idx.data(p[0], p[1], p[2]), p[0], p[1], p[2]);
#endif
				if (!facet_is_degenerated)
					B.add_vertex_to_facet(vertices_idx.data(p[0], p[1], p[2]));
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

CGAL_Polyhedron *createPolyhedronFromPolySet(const PolySet &ps)
{
	CGAL_Polyhedron *P = NULL;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		P = new CGAL_Polyhedron;
		CGAL_Build_PolySet builder(ps);
		P->delegate(builder);
	}
	catch (const CGAL::Assertion_exception &e) {
		PRINTB("CGAL error in CGAL_Build_PolySet: %s", e.what());
		delete P;
		P = NULL;
	}
	CGAL::set_error_behaviour(old_behaviour);
	return P;
}



CGAL_Point_2 project_svg_3to2( CGAL_Point_3 p, CGAL_Iso_cuboid_3 bbox )
{
	// do simple fake isometric projection, based on bounding box
	double x = CGAL::to_double( p.x() );
	double y = CGAL::to_double( p.y() );
	double z = CGAL::to_double( p.z() );
	double screenw = 480;
	double screenh = 480;
	double xcenter = screenw / 2;
	double ycenter = screenh / 2;
	double xdist = ( CGAL::to_double( bbox.xmax() - bbox.xmin() ) );
	double ydist = ( CGAL::to_double( bbox.ymax() - bbox.ymin() ) );
	double zdist = ( CGAL::to_double( bbox.zmax() - bbox.zmin() ) );
	double xscale = (xdist==0) ? 1 : screenw / (xdist * 2 * 1.618);
	double yscale = (ydist==0) ? 1 : screenh / (ydist * 2 * 1.618 * 3);
	double zscale = (zdist==0) ? 1 : screenh / (zdist * 2 * 1.618);
	double tx = xcenter + x * xscale + y * yscale;
	double ty = ycenter - z * zscale - y * yscale;
	return CGAL_Point_2( tx, ty );
}

CGAL_Point_2 project_svg_2to2( CGAL_Point_2 p, CGAL_Iso_rectangle_2 bbox )
{
	// pass thru 3d projection by making 'y' into 'z'.
/*	CGAL_Point_3 origin( 0, 0, 0 );
	CGAL_Iso_cuboid_3 bbox3d( bbox.xmin(), bbox.ymin(), origin.z(),
													  bbox.xmax(), bbox.ymax(), origin.z() );
	CGAL_Point_3 point3d( p.x(), origin.z() , p.y() );
	return project_svg_3to2( point3d, bbox3d );*/
	double x = CGAL::to_double( p.x() );
	double y = CGAL::to_double( p.y() );
	double screenw = 480;
	double screenh = 480;
	double borderw = screenw * 0.1618;
	double borderh = screenh * 0.1618;
	double vizw = screenw - borderw*2;
	double vizh = screenh - borderh*2;
	double bboxw = 0 ; CGAL::to_double( bbox.xmax() - bbox.xmin() );
	double bboxh = 0 ; CGAL::to_double( bbox.ymax() - bbox.ymin() );
	double xinbox = CGAL::to_double( p.x() ) - CGAL::to_double( bbox.xmin() );
	double yinbox = CGAL::to_double( p.y() ) - CGAL::to_double( bbox.ymin() );
	double tx = borderw + ( xinbox / bboxw ) * ( vizw );
	double ty = borderh + ( yinbox / bboxh ) * ( vizh );
	std::cout << "\nx, y " << x << "," << y << "\n";
	std::cout << "xinb, yinb " << xinbox << "," << yinbox << "\n";
	std::cout << "vizw, vizh " << vizw <<  "," << vizh << "\n";
	std::cout << "tx, ty " << tx << "," << ty << "\n";
	return CGAL_Point_2( tx, ty );
}

// for debugging, not necessarily pretty or useful for users.
std::string dump_cgal_nef_polyhedron2_face_svg(
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1,
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c2,
	CGAL_Nef_polyhedron2::Explorer explorer,
	CGAL_Iso_rectangle_2 &bbox,
	std::string color )
{
  std::stringstream out;
	CGAL_For_all(c1, c2) {
		if ( explorer.is_standard( explorer.target(c1) ) ) {
			CGAL_Point_2 source = explorer.point( explorer.source( c1 ) );
			CGAL_Point_2 target = explorer.point( explorer.target( c1 ) );
			CGAL_Point_2 tp1 = project_svg_2to2( source, bbox );
			CGAL_Point_2 tp2 = project_svg_2to2( target, bbox );
      out << "      <line"
			  << " x1='" << CGAL::to_double(tp1.x()) << "'"
			  << " y1='" << CGAL::to_double(tp1.y()) << "'"
			  << " x2='" << CGAL::to_double(tp2.x()) << "'"
			  << " y2='" << CGAL::to_double(tp2.y()) << "'"
			  << " stroke='" << color << "' />\n";
			// crude "arrowhead" to indicate directionality
			out << "      <circle"
			  << " cx='" << CGAL::to_double(tp1.x()+ (tp2.x()-tp1.x())* 7/8) << "'"
			  << " cy='" << CGAL::to_double(tp1.y()+ (tp2.y()-tp1.y())* 7/8) << "'"
			  << " r='2'"
			  << " fill='" << color << "' stroke='" << color << "' />\n";

		}
	}
	return out.str();
}

static int svgcounter=0;

std::string dump_cgal_nef_polyhedron2_svg( const CGAL_Nef_polyhedron2 &N )
{
  std::stringstream out;
  CGAL_Nef_polyhedron2::Explorer explorer = N.explorer();
	CGAL_Iso_rectangle_2 bbox = CGAL::bounding_box ( explorer.points_begin(), explorer.points_end() );

	out << "      <!-- bounding box: "
		<< CGAL::to_double( bbox.xmin() ) << ","
		<< CGAL::to_double( bbox.ymin() ) << " "
		<< CGAL::to_double( bbox.xmax() ) << ","
		<< CGAL::to_double( bbox.ymax() ) << " "
		<< "-->\n";

  CGAL_Nef_polyhedron2::Explorer::Face_const_iterator i;
  out << " <svg y='" << svgcounter << "' width='480px' height='480px' xmlns='http://www.w3.org/2000/svg' version='1.1'>\n";
	out << " <polyline points='0,0 480,0 480,480 0,480' style='fill:none;stroke:black' />\n";
	out << " <polyline points='10,455 10,475 10,465 18,465 2,465 10,465 14,461 6,469 10,465' style='fill:none;stroke:black;' />\n";
	svgcounter+=480;
	if ((svgcounter/480)%2==0) svgcounter += 24;
	for ( i = explorer.faces_begin(); i!= explorer.faces_end(); ++i ) {
			out << "  <!-- body face begin -->\n";
			CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1
				= explorer.face_cycle( i ), c2 ( c1 );
			out << dump_cgal_nef_polyhedron2_face_svg( c1, c2, explorer, bbox, "red" );
			out << "  <!-- body face end -->\n";

		  CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j;
			for ( j = explorer.holes_begin( i ); j!= explorer.holes_end( i ); ++j ) {
				out << "   <!-- hole face begin -->\n";
				CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c3( j ), c4 ( c3 );
				out << dump_cgal_nef_polyhedron2_face_svg( c3, c4, explorer, bbox, "green" );
				out << "   <!-- hole face end -->\n";
			}
  }
  out << "</svg>";
	std::string tmp = out.str();
	boost::replace_all( tmp, "'", "\"" );
	return tmp;
}


std::string dump_cgal_nef_polyhedron2_face(
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1,
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c2,
	CGAL_Nef_polyhedron2::Explorer explorer )
{
  std::stringstream out;
	CGAL_For_all(c1, c2) {
		out << "  On frame edge:" << explorer.is_frame_edge( c1 );
		out << " Mark: " << explorer.mark( c1 );
		if ( explorer.is_standard( explorer.target(c1) ) ) {
			CGAL_Nef_polyhedron2::Explorer::Point source = explorer.point( explorer.source( c1 ) );
			CGAL_Nef_polyhedron2::Explorer::Point target = explorer.point( explorer.target( c1 ) );
      out  << "  Halfedge x y x2 y2: "
				<< CGAL::to_double(source.x()) << " "
				<< CGAL::to_double(source.y()) << " "
				<< CGAL::to_double(target.x()) << " "
				<< CGAL::to_double(target.y()) << "\n";
		} else {
			CGAL_Nef_polyhedron2::Explorer::Ray ray = explorer.ray( explorer.target( c1 ) );
      out  << "  Ray x y dx dy: "
				<< CGAL::to_double(ray.point(0).x()) << " "
				<< CGAL::to_double(ray.point(0).y()) << " "
				<< CGAL::to_double(ray.point(1).x()) << " "
				<< CGAL::to_double(ray.point(1).y()) << "\n";
		}
	}
	return out.str();
}

std::string dump_cgal_nef_polyhedron2( const CGAL_Nef_polyhedron2 &N )
{
  std::stringstream out;
  CGAL_Nef_polyhedron2::Explorer explorer = N.explorer();
  CGAL_Nef_polyhedron2::Explorer::Face_const_iterator i;
  out << "CGAL_Nef_polyhedron2 dump begin\n";
	for ( i = explorer.faces_begin(); i!= explorer.faces_end(); ++i ) {
	    out << " Face body:\n";
			CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1
				= explorer.face_cycle( i ), c2 ( c1 );
			out << dump_cgal_nef_polyhedron2_face( c1, c2, explorer );

		  CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j;
			for ( j = explorer.holes_begin( i ); j!= explorer.holes_end( i ); ++j ) {
		    out << "  Face hole:\n";
				CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c3( j ), c4 ( c3 );
				out << dump_cgal_nef_polyhedron2_face( c3, c4, explorer );
			}
  }
  out << "CGAL_Nef_polyhedron2 dump end";
  return out.str();
}

// This uses the Shell Explorer pattern from the CGAL Manual to dump the 3d Nef Polyhedron information
// http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html#Subsection_29.7.2
class NefPoly3_dumper_svg {
public:
	std::stringstream out;
	CGAL_Iso_cuboid_3 bbox;
	NefPoly3_dumper_svg(const CGAL_Nef_polyhedron3& N) { }
	void setbbox( CGAL_Iso_cuboid_3 bbox ) { this->bbox = bbox; }
	void visit(CGAL_Nef_polyhedron3::Vertex_const_handle v) {}
	void visit(CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SHalfloop_const_handle shh ){}
	void visit(CGAL_Nef_polyhedron3::SFace_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet )
	{
		int contour_count = 0;
		out << "  <!-- Halffacet. Mark: " << (*hfacet).mark() << " -->\n";
		CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator i;
		CGAL_forall_facet_cycles_of( i, hfacet ) {
			CGAL_Nef_polyhedron3::SHalfloop_const_handle shl_handle;
			out << "   <!-- Halffacet cycle: -->\n";
			if ( contour_count == 0 ) {
				out << "    <!-- Body contour:--> \n";
			} else {
				out << "    <!-- Hole contour:--> \n" ;
			}
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(i), c2(c1);
			CGAL_For_all( c1, c2 ) {
				out << "     <line";
				// don't know why we use source()->source(), except thats what CGAL does internally
				CGAL_Point_3 source = c1->source()->source()->point();
				CGAL_Point_3 target = c1->source()->target()->point();
				CGAL_Point_2 tp1 = project_svg_3to2 ( source, bbox );
				CGAL_Point_2 tp2 = project_svg_3to2 ( target, bbox );
				out << " "
					<< "x1='" << CGAL::to_double(tp1.x()) << "' "
					<< "y1='" << CGAL::to_double(tp1.y()) << "' "
					<< "x2='" << CGAL::to_double(tp2.x()) << "' "
					<< "y2='" << CGAL::to_double(tp2.y()) << "' "
				  << "stroke='red' />\n";
			}
			contour_count++;
		} // next facet cycle (i.e. next contour)
	} // visit()

};


std::string dump_cgal_nef_polyhedron3_svg( const CGAL_Nef_polyhedron3 &N )
{
  std::stringstream out;
  out << "<svg width='480px' height='480px' xmlns='http://www.w3.org/2000/svg' version='1.1'>\n";
	out << "<polyline points='0,0 480,0 480,480 0,480' style='fill:none;stroke:black' />\n";
	out << "<polyline points='10,455 10,475 10,465 18,465 2,465 10,465 14,461 6,469 10,465' style='fill:none;stroke:black;' />\n";
	out << "<!--CGAL_Nef_polyhedron3 dump begin-->\n";

	std::vector<CGAL_Point_3> points;
	CGAL_Nef_polyhedron3::Vertex_const_iterator vi;
	for (vi = N.vertices_begin(); vi!=N.vertices_end(); ++vi)
		points.push_back( vi->point() );
	CGAL_Iso_cuboid_3 bbox = CGAL::bounding_box( points.begin(), points.end() );

  CGAL_Nef_polyhedron3::Volume_const_iterator c;
  CGAL_forall_volumes(c,N) {
    out << " <!--Processing volume...-->\n";
    out << "  <!--Mark: " << (*c).mark() << "-->\n";
    CGAL_Nef_polyhedron3::Shell_entry_const_iterator it;
    CGAL_forall_shells_of(it,c) {
      out << "  <!--Processing shell...-->\n";
      NefPoly3_dumper_svg dumper_svg(N);
			dumper_svg.setbbox( bbox );
      N.visit_shell_objects(CGAL_Nef_polyhedron3::SFace_const_handle(it), dumper_svg );
			out << dumper_svg.out.str();
      out << "  <!--Processing shell end-->\n";
    }
    out << " <!--Processing volume end-->\n";
  }
  out << "<!--CGAL_Nef_polyhedron3 dump end-->\n";
	out << "</svg>";
	std::string tmp = out.str();
	boost::replace_all( tmp, "'", "\"" );
  return tmp;
}

// This uses the Shell Explorer pattern from the CGAL Manual to dump the 3d Nef Polyhedron information
// http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html#Subsection_29.7.2
class NefPoly3_dumper {
public:
	std::stringstream out;
	NefPoly3_dumper(const CGAL_Nef_polyhedron3& N) {}
	void visit(CGAL_Nef_polyhedron3::Vertex_const_handle v) {}
	void visit(CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SHalfloop_const_handle shh )
	{
		out << " SHalfloop visit\n";
		out << "  Mark: " << (*shh).mark() << "\n";
	}
	void visit(CGAL_Nef_polyhedron3::SFace_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet )
	{
		int contour_count = 0;
		out << "  Halffacet visit\n";
		out << "   Mark: " << (*hfacet).mark() << "\n";
		CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator i;
		CGAL_forall_facet_cycles_of( i, hfacet ) {
			CGAL_Nef_polyhedron3::SHalfloop_const_handle shl_handle;
			out << "   Halffacet cycle:\n";
			if ( contour_count == 0 ) {
				out << "    Body contour:\n";
			} else {
				out << "    Hole contour:\n" ;
			}
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(i), c2(c1);
			int count=0;
			CGAL_For_all( c1, c2 ) {
				out << "     Halfedge vertex:";
				out << " Mark: " << (*c1).mark();
				count++;
				CGAL_Point_3 point3d = c1->source()->source()->point();
				double x = CGAL::to_double( point3d.x() );
				double y = CGAL::to_double( point3d.y() );
		  	double z = CGAL::to_double( point3d.z() );
			  out << " x:" << x << " y:" << y << " z:" << z <<"\n";
			}
	  		out << "     point count: " << count << "\n";
			contour_count++;
		} // next facet cycle (i.e. next contour)
	} // visit()

};

std::string dump_cgal_nef_polyhedron3( const CGAL_Nef_polyhedron3 &N )
{
  std::stringstream out;
  out << "CGAL_Nef_polyhedron3 dump begin\n";
  CGAL_Nef_polyhedron3::Volume_const_iterator c;
  CGAL_forall_volumes(c,N) {
    out << " Processing volume...\n";
    out << "  Mark: " << (*c).mark() << "\n";
    CGAL_Nef_polyhedron3::Shell_entry_const_iterator it;
    CGAL_forall_shells_of(it,c) {
      out << "  Processing shell...\n";
      NefPoly3_dumper dumper(N);
      N.visit_shell_objects(CGAL_Nef_polyhedron3::SFace_const_handle(it), dumper );
      out << dumper.out.str();
      out << "  Processing shell end\n";
    }
    out << " Processing volume end\n";
  }
  out << "CGAL_Nef_polyhedron3 dump end\n";
  return out.str();
}



#endif /* ENABLE_CGAL */

