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

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "GeometryUtils.h"
#include "dxfdata.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

#include "Reindexer.h"
#include "grid.h"

static void append_geometry(const PolySet &ps, IndexedPolygons &mesh)
{
	GeometryUtils::createIndexedPolygonsFromPolySet(ps, mesh);
}

void append_geometry(const shared_ptr<const Geometry> &geom, IndexedPolygons &mesh)
{
	if (const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(geom.get())) {
		PolySet ps(3);
		bool err = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3), ps);
		if (err) { PRINT("ERROR: Nef->PolySet failed"); }
		else {
			append_geometry(ps, mesh);
		}
	}
	else if (const PolySet *ps = dynamic_cast<const PolySet *>(geom.get())) {
		append_geometry(*ps, mesh);
	}
	else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}
}

void export_off(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	IndexedPolygons mesh;
	append_geometry(geom, mesh);

	output << "OFF " << mesh.vertices.size() << " " << mesh.faces.size() << " 0\n";
	for (const auto &v : mesh.vertices) {
		output << v[0] << " " << v[1] << " " << v[2] << " " << "\n";
	}
	for (const auto &f : mesh.faces) {
		output << f.size();
		for (const auto &i : f) output << " " << i;
		output << "\n";
	}
}

#endif // ENABLE_CGAL
