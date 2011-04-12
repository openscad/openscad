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

#include "cgal.h"
#include <CGAL/convex_hull_2.h>

extern CGAL_Nef_polyhedron2 convexhull2(std::list<CGAL_Nef_polyhedron2> a);
extern CGAL_Poly2 nef2p2(CGAL_Nef_polyhedron2 p);

CGAL_Nef_polyhedron2 convexhull2(std::list<CGAL_Nef_polyhedron2> a)
{
	std::list<CGAL_Nef_polyhedron2::Point> points;

	std::list<CGAL_Nef_polyhedron2>::iterator i;
	for (i=a.begin(); i!=a.end(); i++) {
		CGAL_Poly2 ap=nef2p2(*i);
		for (size_t j=0;j<ap.size();j++) {
	    double x=to_double(ap[j].x()),y=to_double(ap[j].y());
	    CGAL_Nef_polyhedron2::Point p=CGAL_Nef_polyhedron2::Point(x,y);
	    points.push_back(p);
		}
	}

	std::list<CGAL_Nef_polyhedron2::Point> result;
	CGAL::convex_hull_2(points.begin(),points.end(),std::back_inserter(result));

	return CGAL_Nef_polyhedron2(result.begin(),result.end(),CGAL_Nef_polyhedron2::INCLUDED);
}

#endif
