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
#include <boost/variant.hpp>
#include "polyset.h"
#include "dxftess.h"
#include "CGALEvaluator.h"
#include "Tree.h"

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


void CGAL_Nef_polyhedron::transform( const Transform3d &matrix )
{
	if (!this->isNull()) {
		if (this->dim == 2) {
			// Unfortunately CGAL provides no transform method for CGAL_Nef_polyhedron2
			// objects. So we convert in to our internal 2d data format, transform it,
			// tesselate it and create a new CGAL_Nef_polyhedron2 from it.. What a hack!
			Eigen::Matrix2f testmat;
			testmat << matrix(0,0), matrix(0,1), matrix(1,0), matrix(1,1);
			if (testmat.determinant() == 0) {
				PRINT("Warning: Scaling a 2D object with 0 - removing object");
				this->reset();
				return;
			}
			else {
				CGAL_Aff_transformation2 t(
					matrix(0,0), matrix(0,1), matrix(0,3),
					matrix(1,0), matrix(1,1), matrix(1,3), matrix(3,3));

				DxfData *dd = this->convertToDxfData();
				for (size_t i=0; i < dd->points.size(); i++) {
					CGAL_Kernel2::Point_2 p = CGAL_Kernel2::Point_2(dd->points[i][0], dd->points[i][1]);
					p = t.transform(p);
					dd->points[i][0] = to_double(p.x());
					dd->points[i][1] = to_double(p.y());
				}

				PolySet ps;
				ps.is2d = true;
				dxf_tesselate(&ps, *dd, 0, Vector2d(1,1), true, false, 0);

				Tree nulltree;
				CGALEvaluator tmpeval(nulltree);
				CGAL_Nef_polyhedron N = tmpeval.evaluateCGALMesh(ps);
				if ( N.p2 ) this->p2.reset( new CGAL_Nef_polyhedron2( *N.p2 ) );
				delete dd;
			}
		}
		else if (this->dim == 3) {
			if (matrix.matrix().determinant() == 0) {
				PRINT("Warning: Scaling a 3D object with 0 - removing object");
				this->reset();
			}
			else {
				CGAL_Aff_transformation t(
					matrix(0,0), matrix(0,1), matrix(0,2), matrix(0,3),
					matrix(1,0), matrix(1,1), matrix(1,2), matrix(1,3),
					matrix(2,0), matrix(2,1), matrix(2,2), matrix(2,3), matrix(3,3));
				this->p3->transform(t);
			}
		}
	}
}

#endif // ENABLE_CGAL
