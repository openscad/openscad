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

#ifdef ENABLE_CGAL

#include "printutils.h"
#include "grid.h"
#include "cgal.h"

extern CGAL_Poly2 nef2p2(CGAL_Nef_polyhedron2 p);

//-----------------------------------------------------------------------------
// Pretty-print a CGAL polygon.
//
template<class Kernel, class Container>
void print_polygon (const CGAL::Polygon_2<Kernel, Container>& P)
{
  typename CGAL::Polygon_2<Kernel, Container>::Vertex_const_iterator  vit;

  std::cout << "[ " << P.size() << " vertices:";
  for (vit = P.vertices_begin(); vit != P.vertices_end(); ++vit)
    std::cout << " (" << *vit << ')';
  std::cout << " ]" << std::endl;
}

//-----------------------------------------------------------------------------
// Pretty-print a polygon with holes.
//
template<class Kernel, class Container>
void print_polygon_with_holes (const CGAL::Polygon_with_holes_2<Kernel, Container>& pwh) { 
  if (! pwh.is_unbounded()) { 
		std::cout << "{ Outer boundary = ";
		print_polygon (pwh.outer_boundary());
	} else
    std::cout << "{ Unbounded polygon." << std::endl;

  typename CGAL::Polygon_with_holes_2<Kernel,Container>::Hole_const_iterator  hit;
  unsigned int k = 1;

  std::cout << "  " << pwh.number_of_holes() << " holes:" << std::endl;
  for (hit = pwh.holes_begin(); hit != pwh.holes_end(); ++hit, ++k) { 
    std::cout << "    Hole #" << k << " = ";
    print_polygon (*hit);
  }
  std::cout << " }" << std::endl;

  return;
}

CGAL_Poly2 nef2p2(CGAL_Nef_polyhedron2 p)
{
	std::list<CGAL_ExactKernel2::Point_2> points;
	Grid2d<int> grid(GRID_COARSE);

	typedef CGAL_Nef_polyhedron2::Explorer Explorer;
	typedef Explorer::Face_const_iterator fci_t;
	typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
	Explorer E = p.explorer();

	for (fci_t fit = E.faces_begin(), facesend = E.faces_end(); fit != facesend; ++fit)
	{
		if (!E.mark(fit)) {
			continue;
		}
		//if (fit != E.faces_begin()) {
		if (points.size() != 0) {
			PRINT("WARNING: minkowski() and hull() is not implemented for 2d objects with holes!");
			break;
		}

		heafcc_t fcirc(E.halfedge(fit)), fend(fcirc);
		CGAL_For_all(fcirc, fend) {
			if (E.is_standard(E.target(fcirc))) {
				Explorer::Point ep = E.point(E.target(fcirc));
				double x = to_double(ep.x()), y = to_double(ep.y());
				grid.align(x, y);
				points.push_back(CGAL_ExactKernel2::Point_2(x, y));
			}
		}
	}

	return CGAL_Poly2(points.begin(), points.end());
}
static CGAL_Nef_polyhedron2 p2nef2(CGAL_Poly2 p2) {
  std::list<CGAL_Nef_polyhedron2::Point> points;
  for (size_t j = 0; j < p2.size(); j++) {
    double x = to_double(p2[j].x());
    double y = to_double(p2[j].y());
    CGAL_Nef_polyhedron2::Point p = CGAL_Nef_polyhedron2::Point(x, y);
    points.push_back(p);
  }
  return CGAL_Nef_polyhedron2(points.begin(), points.end(), CGAL_Nef_polyhedron2::INCLUDED);
}

CGAL_Nef_polyhedron2 minkowski2(const CGAL_Nef_polyhedron2 &a, const CGAL_Nef_polyhedron2 &b)
{
	CGAL_Poly2 ap = nef2p2(a), bp = nef2p2(b);

	if (ap.size() == 0) {
		PRINT("WARNING: minkowski() could not get any points from object 1!");
		return CGAL_Nef_polyhedron2();
	} else if (bp.size() == 0) {
		PRINT("WARNING: minkowski() could not get any points from object 2!");
		return CGAL_Nef_polyhedron2();
	} else {
		CGAL_Poly2h x = minkowski_sum_2(ap, bp);

		// Make a CGAL_Nef_polyhedron2 out of just the boundary for starters
		return p2nef2(x.outer_boundary());
	}	
}

#endif

