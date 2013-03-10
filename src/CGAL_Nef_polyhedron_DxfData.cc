/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "dxfdata.h"
#include "grid.h"
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

#ifdef ENABLE_CGAL

DxfData *CGAL_Nef_polyhedron::convertToDxfData() const
{
	assert(this->dim == 2);
	DxfData *dxfdata = new DxfData();
	Grid2d<int> grid(GRID_COARSE);

	typedef CGAL_Nef_polyhedron2::Explorer Explorer;
	typedef Explorer::Face_const_iterator fci_t;
	typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
	Explorer E = this->p2->explorer();

	for (fci_t fit = E.faces_begin(), facesend = E.faces_end(); fit != facesend; ++fit)
	{
		heafcc_t fcirc(E.halfedge(fit)), fend(fcirc);
		int first_point = -1, last_point = -1;
		CGAL_For_all(fcirc, fend) {
			if (E.is_standard(E.target(fcirc))) {
				Explorer::Point ep = E.point(E.target(fcirc));
				double x = to_double(ep.x()), y = to_double(ep.y());
				int this_point = -1;
				if (grid.has(x, y)) {
					this_point = grid.align(x, y);
				} else {
					this_point = grid.align(x, y) = dxfdata->points.size();
					dxfdata->points.push_back(Vector2d(x, y));
				}
				if (first_point < 0) {
					dxfdata->paths.push_back(DxfData::Path());
					first_point = this_point;
				}
				if (this_point != last_point) {
					dxfdata->paths.back().indices.push_back(this_point);
					last_point = this_point;
				}
			}
		}
		if (first_point >= 0) {
			dxfdata->paths.back().is_closed = 1;
			dxfdata->paths.back().indices.push_back(first_point);
		}
	}

	dxfdata->fixup_path_direction();
	return dxfdata;
}

std::string CGAL_Nef_polyhedron::dump() const
{
	if (this->dim==2)
		return OpenSCAD::dump_svg( *this->p2 );
	else if (this->dim==3)
		return OpenSCAD::dump_svg( *this->p3 );
	else
		return std::string("Nef Polyhedron with dimension != 2 or 3");
}

// use a try/catch block around any calls to this
void CGAL_Nef_polyhedron::convertTo2d()
{
	logstream log(5);
	if (dim!=3) return;
	assert(this->p3);
	ZRemover zremover;
	CGAL_Nef_polyhedron3::Volume_const_iterator i;
	CGAL_Nef_polyhedron3::Shell_entry_const_iterator j;
	CGAL_Nef_polyhedron3::SFace_const_handle sface_handle;
	for ( i = this->p3->volumes_begin(); i != this->p3->volumes_end(); ++i ) {
		log << "<!-- volume begin. mark: " << i->mark() << " -->\n";
		for ( j = i->shells_begin(); j != i->shells_end(); ++j ) {
			log << "<!-- shell. mark: " << i->mark() << " -->\n";
			sface_handle = CGAL_Nef_polyhedron3::SFace_const_handle( j );
			this->p3->visit_shell_objects( sface_handle , zremover );
			log << "<!-- shell. end. -->\n";
		}
		log << "<!-- volume end. -->\n";
	}
	this->p3.reset();
	this->p2 = zremover.output_nefpoly2d;
	this->dim = 2;
}


std::vector<CGAL_Point_3> face2to3(
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1,
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c2,
	CGAL_Nef_polyhedron2::Explorer explorer )
{
	std::vector<CGAL_Point_3> result;
	CGAL_For_all(c1, c2) {
		if ( explorer.is_standard( explorer.target(c1) ) ) {
			//CGAL_Point_2e source = explorer.point( explorer.source( c1 ) );
			CGAL_Point_2e target = explorer.point( explorer.target( c1 ) );
			if (c1->mark()) {
				CGAL_Point_3 tmp( target.x(), target.y(), 0 );
				result.push_back( tmp );
			}
		}
	}
	return result;
}


// use a try/catch block around any calls to this
void CGAL_Nef_polyhedron::convertTo3d()
{
	if (dim!=2) return;
	assert(this->p2);
  CGAL_Nef_polyhedron2::Explorer explorer = this->p2->explorer();
  CGAL_Nef_polyhedron2::Explorer::Face_const_iterator i;

	this->p3.reset( new CGAL_Nef_polyhedron3() );

	for ( i = explorer.faces_begin(); i!= explorer.faces_end(); ++i ) {

		CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1
			= explorer.face_cycle( i ), c2 ( c1 );
		std::vector<CGAL_Point_3> body_pts = face2to3( c1, c2, explorer );
		CGAL_Nef_polyhedron3 body( body_pts.begin(), body_pts.end() );

		CGAL_Nef_polyhedron3 holes;
	  CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j;
		for ( j = explorer.holes_begin( i ); j!= explorer.holes_end( i ); ++j ) {
			CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c3( j ), c4 ( c3 );
			std::vector<CGAL_Point_3> hole_pts = face2to3( c3, c4, explorer );
			CGAL_Nef_polyhedron3 hole( hole_pts.begin(), hole_pts.end() );
			holes = holes.join( hole );
		}

		body = body.difference( holes );
		*(this->p3) = this->p3->join( body );
  }

	this->p2.reset();
	this->dim = 3;
}


#endif // ENABLE_CGAL
