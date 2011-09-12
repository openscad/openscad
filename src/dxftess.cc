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

#include "printutils.h"

#ifdef ENABLE_CGAL
#include "dxftess-cgal.cc"
#else
#include "dxftess-glu.cc"
#endif

/*!
	Converts all paths in the given DxfData to PolySet::borders polygons
	without tesselating. Vertex ordering of the resulting polygons
	will follow the paths' is_inner flag.
*/
void dxf_border_to_ps(PolySet *ps, const DxfData &dxf)
{
	for (size_t i = 0; i < dxf.paths.size(); i++) {
		const DxfData::Path &path = dxf.paths[i];
		if (!path.is_closed)
			continue;
		ps->borders.push_back(PolySet::Polygon());
		for (size_t j = 1; j < path.indices.size(); j++) {
			double x = dxf.points[path.indices[j]][0], y = dxf.points[path.indices[j]][1], z = 0.0;
			ps->grid.align(x, y, z);
			if (path.is_inner) {
				ps->borders.back().push_back(Vector3d(x, y, z));
			} else {
				ps->borders.back().insert(ps->borders.back().begin(), Vector3d(x, y, z));
			}
		}
	}
}
