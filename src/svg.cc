#ifdef ENABLE_CGAL
#include "cgalutils.h"
#include "svg.h"
#include <boost/algorithm/string.hpp>
#include <map>

namespace OpenSCAD {

// SVG code
// currently for debugging, not necessarily pretty or useful for users. (yet)
int svg_cursor_py = 0;
int svg_px_width = SVG_PXW;
int svg_px_height = SVG_PXH;

std::string svg_header( int widthpx, int heightpx )
{
	std::stringstream out;
	out << "<svg width='" << widthpx << "px' height='" << heightpx << "px'"
		<< " xmlns='http://www.w3.org/2000/svg' version='1.1'>";
	return out.str();
}

std::string svg_label(std::string s)
{
	std::stringstream out;
	out << "   <text fill='black' x='20' y='40' font-size='24'>" << s << "</text>";
	return out.str();
}

std::string svg_border()
{
	std::stringstream out;
	out << " <!-- border -->\n";
	out << "  <polyline points='0,0 "
		<< svg_px_width  << ",0 "
		<< svg_px_width  << "," << svg_px_height
		<< " 0," << svg_px_height << "'"
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

CGAL_Nef_polyhedron2::Explorer::Point project_svg_3to2( CGAL_Point_3 p, CGAL_Iso_cuboid_3 bbox )
{
	CGAL_Kernel3::FT screenw(svg_px_width);
	CGAL_Kernel3::FT screenh(svg_px_height);
	CGAL_Kernel3::FT screenxc = screenw / 2;
	CGAL_Kernel3::FT screenyc = screenh / 2;
	CGAL_Kernel3::FT bboxx = ( bbox.xmax() - bbox.xmin() );
	CGAL_Kernel3::FT bboxy = ( bbox.ymax() - bbox.ymin() );
	CGAL_Kernel3::FT bboxz = ( bbox.zmax() - bbox.zmin() );
	CGAL_Kernel3::FT largest_dim = CGAL::max( CGAL::max( bboxx, bboxy ), bboxz );
	CGAL_Kernel3::FT bboxxc = bboxx / 2 + bbox.xmin();
	CGAL_Kernel3::FT bboxyc = bboxy / 2 + bbox.ymin();
	CGAL_Kernel3::FT bboxzc = bboxz / 2 + bbox.zmin();
	CGAL_Kernel3::FT xinbox = ( p.x() - bboxxc ) / largest_dim;
	CGAL_Kernel3::FT yinbox = ( p.y() - bboxyc ) / largest_dim;
	CGAL_Kernel3::FT zinbox = ( p.z() - bboxzc ) / largest_dim;
	// do simple fake paralell projection
	CGAL_Kernel3::FT tx = screenxc + xinbox * screenw / 1.618 + yinbox * screenh / 3.2;
	CGAL_Kernel3::FT ty = screenyc - zinbox * screenh / 1.618 - yinbox * screenh / 3.2;
	return CGAL_Point_2e(CGAL::to_double(tx), CGAL::to_double(ty));
}

CGAL_Point_2e project_svg_2to2(const CGAL_Point_2e &p, const CGAL_Iso_rectangle_2e &bbox)
{
	double screenw = svg_px_width;
	double screenh = svg_px_height;
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
	return CGAL_Point_2e( tx, ty );
}

std::string dump_cgal_nef_polyhedron2_face_svg(
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1,
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c2,
	CGAL_Nef_polyhedron2::Explorer explorer,
	std::string color,
	bool mark,
	CGAL_Iso_rectangle_2e bbox )
{
  std::stringstream out;
	CGAL_For_all(c1, c2) {
		if ( explorer.is_standard( explorer.target(c1) ) ) {
CGAL_Nef_polyhedron2::Explorer::Point source = explorer.point( explorer.source( c1 ) );
			CGAL_Point_2e target = explorer.point( explorer.target( c1 ) );
			CGAL_Point_2e tp1 = project_svg_2to2( source, bbox );
			CGAL_Point_2e tp2 = project_svg_2to2( target, bbox );
			double mod=0;
			if (color=="green") mod=10;
			out << "      <!-- Halfedge. Mark: " << c1->mark() << " -->\n";
      out << "       <line"
			  << " x1='" << CGAL::to_double(tp1.x()) + mod << "'"
			  << " y1='" << CGAL::to_double(tp1.y()) - mod << "'"
			  << " x2='" << CGAL::to_double(tp2.x()) + mod << "'"
			  << " y2='" << CGAL::to_double(tp2.y()) - mod << "'"
			  << " stroke='" << color << "'";
			if (!mark) out << " stroke-dasharray='4 4' />\n";
			else out << " />\n";
			// crude "arrowhead" to indicate directionality
			out << "       <circle"
			  << " cx='" << CGAL::to_double(tp1.x()+ (tp2.x()-tp1.x())* 7/8) + mod << "'"
			  << " cy='" << CGAL::to_double(tp1.y()+ (tp2.y()-tp1.y())* 7/8) - mod << "'"
			  << " r='2'"
			  << " fill='" << color << "' stroke='" << color << "' />\n";
		} else {
			out << "       <!-- 2d Nef Rays - not implemented -->\n";
		}
	}
	return out.str();
}

std::string dump_svg( const CGAL_Nef_polyhedron2 &N )
{
  std::stringstream out;
  CGAL_Nef_polyhedron2::Explorer explorer = N.explorer();

	CGAL_Iso_rectangle_2e bbox = bounding_box( N );

  CGAL_Nef_polyhedron2::Explorer::Face_const_iterator i;
  out << " <svg y='" << svg_cursor_py << "' width='" << svg_px_width
		<< "' height='" << svg_px_height
		<< "' xmlns='http://www.w3.org/2000/svg' version='1.1'>\n";
	out << svg_border() << "\n" << svg_axes() << "\n";
	svg_cursor_py += svg_px_height;

	for ( i = explorer.faces_begin(); i!= explorer.faces_end(); ++i ) {
			out << "  <!-- face begin. mark: " << i->mark() << "  -->\n";
			CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1
				= explorer.face_cycle( i ), c2 ( c1 );
			out << dump_cgal_nef_polyhedron2_face_svg( c1, c2, explorer, "red", i->mark(), bbox );

		  CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j;
			for ( j = explorer.holes_begin( i ); j!= explorer.holes_end( i ); ++j ) {
				out << "   <!-- hole begin. mark: " << j->mark() << " -->\n";
				CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c3( j ), c4 ( c3 );
				out << dump_cgal_nef_polyhedron2_face_svg( c3, c4, explorer, "green", j->mark(), bbox );
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
	void visit(CGAL_Nef_polyhedron3::Vertex_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SHalfloop_const_handle ) {}
	void visit(CGAL_Nef_polyhedron3::SFace_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet )
	{
		int contour_count = 0;
		out << "  <!-- Halffacet. Mark: " << (*hfacet).mark() << " -->\n";
		std::string color = "gold";
		if (!(*hfacet).mark()) color = "green";
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
				CGAL_Point_2e tp1 = project_svg_3to2 ( source, bbox );
				CGAL_Point_2e tp2 = project_svg_3to2 ( target, bbox );
				out << " "
					<< "x1='" << CGAL::to_double(tp1.x()) << "' "
					<< "y1='" << CGAL::to_double(tp1.y()) << "' "
					<< "x2='" << CGAL::to_double(tp2.x()) << "' "
					<< "y2='" << CGAL::to_double(tp2.y()) << "' "
				  << " stroke='" << color << "'";
				if (!(*hfacet).mark()) out << " stroke-dasharray='4 4' />\n";
				else out << " />\n";
			}
			contour_count++;
		} // next facet cycle (i.e. next contour)
	} // visit()

};


std::string dump_svg( const CGAL_Nef_polyhedron3 &N )
{
  std::stringstream out;
	out << svg_header() << "\n" << svg_border() << "\n" << svg_axes() << "\n";
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

} // namespace

#endif // ENABLE_CGAL
