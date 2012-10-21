#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "cgal.h"
#include <CGAL/bounding_box.h>
typedef CGAL::Point_3<CGAL_Kernel3> CGAL_Point_3;
typedef CGAL::Iso_cuboid_3<CGAL_Kernel3> CGAL_Iso_cuboid_3;

typedef CGAL_Nef_polyhedron2::Explorer::Point CGAL_Point_2;
// Iso_rectangle is different- CGAL_Kernel2::Point != CGAL_Nef2::Explorer::Point
typedef CGAL::Iso_rectangle_2< CGAL::Simple_cartesian<NT> > CGAL_Iso_rectangle_2;

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


std::string svg_header()
{
	std::stringstream out;
  out << "<svg width='480px' height='480px'"
		<< " xmlns='http://www.w3.org/2000/svg' version='1.1'>";
	return out.str();
}

std::string svg_border()
{
	std::stringstream out;
	out << " <!-- border -->\n";
	out << "  <polyline points='0,0 480,0 480,480 0,480'"
		<< " style='fill:none;stroke:black' />\n";
	out << " <!-- /border -->";
	return out.str();
}

std::string svg_axes()
{
	std::stringstream out;
	out << " <!-- axes -->\n";
	out << "  <polyline points='10,455 10,475 10,465 18,465 2,465 10,465 14,461 6,469 10,465'"
		<< " style='fill:none;stroke:black;' />\n";
	out << " <!-- /axes -->";
	return out.str();
}

CGAL_Iso_cuboid_3 bounding_box( const CGAL_Nef_polyhedron3 &N )
{
	CGAL_Iso_cuboid_3 result(-1,-1,-1,1,1,1);
	CGAL_Nef_polyhedron3::Vertex_const_iterator vi;
	std::vector<CGAL_Nef_polyhedron3::Point_3> points;
	CGAL_forall_vertices( vi, N )
		points.push_back( vi->point() );
	if (points.size())
		result = CGAL::bounding_box( points.begin(), points.end() );
	return result;
}

CGAL_Iso_rectangle_2 bounding_box( const CGAL_Nef_polyhedron2 &N )
{
	CGAL_Iso_rectangle_2 result(-1,-1,1,1);
  CGAL_Nef_polyhedron2::Explorer explorer = N.explorer();
	CGAL_Nef_polyhedron2::Explorer::Vertex_const_iterator vi;
	std::vector<CGAL_Point_2> points;
	for ( vi = explorer.vertices_begin(); vi != explorer.vertices_end(); ++vi )
		if ( explorer.is_standard( vi ) )
			points.push_back( explorer.point( vi ) );
	if (points.size())
		result = CGAL::bounding_box( points.begin(), points.end() );
	return result;
}

CGAL_Point_2 project_svg_3to2( CGAL_Point_3 p, CGAL_Iso_cuboid_3 bbox )
{
	// do simple fake isometric projection
	double x = CGAL::to_double( p.x() );
	double y = CGAL::to_double( p.y() );
	double z = CGAL::to_double( p.z() );
	double screenw = 480;
	double screenh = 480;
	double borderw = screenw * 0.1618;
	double borderh = screenh * 0.1618;
	double vizw = screenw - borderw*2;
	double vizh = screenh - borderh*2;
	double bboxx = CGAL::to_double( bbox.xmax() - bbox.xmin() );
	double bboxy = CGAL::to_double( bbox.ymax() - bbox.ymin() );
	double bboxz = CGAL::to_double( bbox.zmax() - bbox.zmin() );
	double xinbox = CGAL::to_double( p.x() ) - CGAL::to_double( bbox.xmin() );
	double yinbox = CGAL::to_double( p.y() ) - CGAL::to_double( bbox.ymin() );
	double zinbox = CGAL::to_double( p.z() ) - CGAL::to_double( bbox.zmin() );
	double tx = borderw + ( xinbox / ( bboxx==0?1:bboxx ) ) * ( vizw );
	double ty = screenh - borderh - ( zinbox / ( bboxz==0?1:bboxz ) ) * ( vizh );
	tx += ( yinbox / ( bboxy==0?1:bboxy ) ) / 3;
	ty -= ( yinbox / ( bboxy==0?1:bboxy ) ) / 3;
	return CGAL_Point_2( tx, ty );
}

CGAL_Point_2 project_svg_2to2( CGAL_Point_2 p, CGAL_Iso_rectangle_2 bbox )
{
	double x = CGAL::to_double( p.x() );
	double y = CGAL::to_double( p.y() );
	double screenw = 480;
	double screenh = 480;
	double borderw = screenw * 0.1618;
	double borderh = screenh * 0.1618;
	double vizw = screenw - borderw*2;
	double vizh = screenh - borderh*2;
	double bboxw = CGAL::to_double( bbox.xmax() - bbox.xmin() );
	double bboxh = CGAL::to_double( bbox.ymax() - bbox.ymin() );
	double xinbox = CGAL::to_double( p.x() ) - CGAL::to_double( bbox.xmin() );
	double yinbox = CGAL::to_double( p.y() ) - CGAL::to_double( bbox.ymin() );
	double tx = borderw + ( xinbox / ( bboxw==0?1:bboxw ) ) * ( vizw );
	double ty = screenh - borderh - ( yinbox / ( bboxh==0?1:bboxh ) ) * ( vizh );
/*	std::cout << "\nx, y " << x << "," << y << "\n";
	std::cout << "bbw, bbh " << bboxw << "," << bboxh << "\n";
	std::cout << "xinb, yinb " << xinbox << "," << yinbox << "\n";
	std::cout << "vizw, vizh " << vizw <<  "," << vizh << "\n";
	std::cout << "tx, ty " << tx << "," << ty << "\n";
*/
	return CGAL_Point_2( tx, ty );
}

// for debugging, not necessarily pretty or useful for users.
std::string dump_cgal_nef_polyhedron2_face_svg(
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1,
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c2,
	CGAL_Nef_polyhedron2::Explorer explorer,
	std::string color,
	CGAL_Iso_rectangle_2 bbox )
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

std::string dump_cgal_nef_polyhedron2_svg( const CGAL_Nef_polyhedron2 &N )
{
  std::stringstream out;
  CGAL_Nef_polyhedron2::Explorer explorer = N.explorer();

	CGAL_Iso_rectangle_2 bbox = bounding_box( N );

  CGAL_Nef_polyhedron2::Explorer::Face_const_iterator i;
  out << " <svg y='" << svg_counter << "' width='480px' height='480px' xmlns='http://www.w3.org/2000/svg' version='1.1'>\n";
	out << svg_border() << "\n" << svg_axes() << "\n";
	svg_counter+=480;
	if ((svg_counter/480)%2==0) svg_counter += 24;
	for ( i = explorer.faces_begin(); i!= explorer.faces_end(); ++i ) {
			out << "  <!-- face begin -->\n";
			CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1
				= explorer.face_cycle( i ), c2 ( c1 );
			out << dump_cgal_nef_polyhedron2_face_svg( c1, c2, explorer, "red", bbox );

		  CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j;
			for ( j = explorer.holes_begin( i ); j!= explorer.holes_end( i ); ++j ) {
				out << "   <!-- hole begin -->\n";
				CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c3( j ), c4 ( c3 );
				out << dump_cgal_nef_polyhedron2_face_svg( c3, c4, explorer, "green", bbox );
				out << "   <!-- hole end -->\n";
			}
			out << "  <!-- face end -->\n";
  }
  out << "</svg>";
	std::string tmp = out.str();
	boost::replace_all( tmp, "'", "\"" );
	return tmp;
}


// This uses the Shell Explorer pattern from the CGAL Manual to dump the 3d Nef Polyhedron information
// http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html#Subsection_29.7.2
class NefPoly3_dumper_svg {
public:
	std::stringstream out;
	CGAL_Iso_cuboid_3 bbox;
	NefPoly3_dumper_svg(const CGAL_Nef_polyhedron3& N)
	{
		bbox = bounding_box( N );
	}
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
	out << svg_header << "\n" << svg_border() << "\n" << svg_axes() << "\n";
	out << "<!--CGAL_Nef_polyhedron3 dump begin-->\n";

  CGAL_Nef_polyhedron3::Volume_const_iterator c;
  CGAL_forall_volumes(c,N) {
    out << " <!--Processing volume...-->\n";
    out << "  <!--Mark: " << (*c).mark() << "-->\n";
    CGAL_Nef_polyhedron3::Shell_entry_const_iterator it;
    CGAL_forall_shells_of(it,c) {
      out << "  <!--Processing shell...-->\n";
      NefPoly3_dumper_svg dumper_svg(N);
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


#endif /* ENABLE_CGAL */

