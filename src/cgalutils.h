#ifndef CGALUTILS_H_
#define CGALUTILS_H_

#include <cgalfwd.h>
class PolySet *createPolySetFromPolyhedron(const CGAL_Polyhedron &p);
CGAL_Polyhedron *createPolyhedronFromPolySet(const class PolySet &ps);
CGAL_Iso_cuboid_3 bounding_box( const CGAL_Nef_polyhedron3 &N );
CGAL_Iso_rectangle_2e bounding_box( const CGAL_Nef_polyhedron2 &N );

#include "svg.h"
#include "printutils.h"

/*

ZRemover

This class converts one or more Nef3 polyhedra into a Nef2 polyhedron by
stripping off the 'z' coordinates from the vertices. The resulting Nef2
poly is accumulated in the 'output_nefpoly2d' member variable.

The 'z' coordinates will either be all 0s, for an xy-plane intersected Nef3,
or, they will be a mixture of -eps and +eps, for a thin-box intersected Nef3.

Notes on CGAL's Nef Polyhedron2:

1. The 'mark' on a 2d Nef face is important when doing unions/intersections.
 If the 'mark' of a face is wrong the resulting nef2 poly will be unexpected.
2. The 'mark' can be dependent on the points fed to the Nef2 constructor.
 This is why we iterate through the 3d faces using the halfedge cycle
 source()->target() instead of the ordinary source()->source(). The
 the latter can generate sequences of points that will fail the
 the CGAL::is_simple_2() test, resulting in improperly marked nef2 polys.
3. 3d facets have 'two sides'. we throw out the 'down' side to prevent dups.

The class uses the 'visitor' pattern from the CGAL manual. See also
http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html
http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3_ref/Class_Nef_polyhedron3.html
OGL_helper.h
*/

class ZRemover {
public:
	logstream log;
	CGAL_Nef_polyhedron2::Boundary boundary;
	boost::shared_ptr<CGAL_Nef_polyhedron2> tmpnef2d;
	boost::shared_ptr<CGAL_Nef_polyhedron2> output_nefpoly2d;
	CGAL::Direction_3<CGAL_Kernel3> up;
	ZRemover()
	{
		output_nefpoly2d.reset( new CGAL_Nef_polyhedron2() );
		boundary = CGAL_Nef_polyhedron2::INCLUDED;
		up = CGAL::Direction_3<CGAL_Kernel3>(0,0,1);
		log = logstream(5);
	}
	void visit( CGAL_Nef_polyhedron3::Vertex_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SHalfloop_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SFace_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet ) {
		log << " <!-- Halffacet visit. Mark: " << hfacet->mark() << " -->\n";
		if ( hfacet->plane().orthogonal_direction() != this->up ) {
			log << "  <!-- down-facing half-facet. skipping -->\n";
			log << " <!-- Halffacet visit end-->\n";
			return;
		}

		// possible optimization - throw out facets that are 'side facets' between
		// the top & bottom of the big thin box. (i.e. mixture of z=-eps and z=eps)

		CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator fci;
		int contour_counter = 0;
		CGAL_forall_facet_cycles_of( fci, hfacet ) {
			if ( fci.is_shalfedge() ) {
				CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(fci), cend(c1);
				std::vector<CGAL_Nef_polyhedron2::Explorer::Point> contour;
				CGAL_For_all( c1, cend ) {
					CGAL_Nef_polyhedron3::Point_3 point3d = c1->source()->target()->point();
					CGAL_Nef_polyhedron2::Explorer::Point point2d( point3d.x(), point3d.y() );
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

				log << "\n<!-- ======== output tmp nef: ==== -->\n"
					<< OpenSCAD::dump_svg( *tmpnef2d ) << "\n"
					<< "\n<!-- ======== output accumulator: ==== -->\n"
					<< OpenSCAD::dump_svg( *output_nefpoly2d ) << "\n";

				contour_counter++;
			} else {
				log << " <!-- trivial facet cycle skipped -->\n";
			}
		} // next facet cycle (i.e. next contour)
		log << " <!-- Halffacet visit end -->\n";
	} // visit()
};


#endif
