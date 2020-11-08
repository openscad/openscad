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
#include "dxfdata.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

#include "grid.h"

static void append_geometry(const PolySet &ps, PolySet &mesh)
{
	mesh.append(ps);
}

void append_geometry(const shared_ptr<const Geometry> &geom, PolySet &mesh)
{
	if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		for (const Geometry::GeometryItem &item : geomlist->getChildren()) {
			append_geometry(item.second, mesh);
		}
	}
	else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		PolySet ps(3);
		bool err = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3), ps);
		if (err) { 
			LOG(message_group::Error,Location::NONE,"","Nef->PolySet failed");
		}
		else {
			append_geometry(ps, mesh);
		}
	}
	else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
		append_geometry(*ps, mesh);
	}
	else if (dynamic_pointer_cast<const Polygon2d>(geom)) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}
}

void export_off(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	PolySet mesh(3,unknown);
	append_geometry(geom, mesh);

	std::vector<Vector3f> vertices;
	mesh.getVertices<Vector3f>(vertices);

	output << "OFF " << vertices.size() << " " << mesh.getIndexedTriangles().size() << " 0\n";
	size_t numverts = vertices.size();
	for (const auto &v : vertices) {
		output << v[0] << " " << v[1] << " " << v[2] << " " << "\n";		
	}
	for (const auto &t : mesh.getIndexedTriangles()) {
		output << t.size();
		output << " " << t[0] << " " << t[1] << " " << t[2];
		output << "\n";
	}
}

#endif // ENABLE_CGAL
