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

#include "node.h"
#include "printutils.h"
#include "grid.h"
#include "cgal.h"

#if 0

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/minkowski_sum_2.h>

extern CGAL_Nef_polyhedron2 minkowski2(CGAL_Nef_polyhedron2 a, CGAL_Nef_polyhedron2 b);

struct K2 : public CGAL::Exact_predicates_exact_constructions_kernel {};
typedef CGAL::Polygon_2<K2> Poly2;
typedef CGAL::Polygon_with_holes_2<K2> Poly2h;

static Poly2 nef2p2(CGAL_Nef_polyhedron2 p)
{
	std::list<K2::Point_2> points;
	Grid2d<int> grid(GRID_COARSE);

	typedef CGAL_Nef_polyhedron2::Explorer Explorer;
	typedef Explorer::Face_const_iterator fci_t;
	typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
	Explorer E = p.explorer();

	for (fci_t fit = E.faces_begin(), fend = E.faces_end(); fit != fend; ++fit)
	{
		if (fit != E.faces_begin()) {
			PRINT("WARNING: minkowski() is not implemented for 2d objects with holes!");
			break;
		}

		heafcc_t fcirc(E.halfedge(fit)), fend(fcirc);
		CGAL_For_all(fcirc, fend) {
			if (E.is_standard(E.target(fcirc))) {
				Explorer::Point ep = E.point(E.target(fcirc));
				double x = to_double(ep.x()), y = to_double(ep.y());
				grid.align(x, y);
				points.push_back(K2::Point_2(x, y));
			}
		}
	}

	return Poly2(points.begin(), points.end());
}

CGAL_Nef_polyhedron2 minkowski2(CGAL_Nef_polyhedron2 a, CGAL_Nef_polyhedron2 b)
{
	Poly2 ap = nef2p2(a), bp = nef2p2(b);
	Poly2h x = minkowski_sum_2(ap, bp);
	/** FIXME **/
	
	PRINT("WARNING: minkowski() is not implemented yet for 2d objects!");
	return CGAL_Nef_polyhedron2();
}

#else

CGAL_Nef_polyhedron2 minkowski2(CGAL_Nef_polyhedron2, CGAL_Nef_polyhedron2)
{
	PRINT("WARNING: minkowski() is not implemented yet for 2d objects!");
	return CGAL_Nef_polyhedron2();
}

#endif

#endif

