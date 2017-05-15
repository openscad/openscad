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

#include <algorithm>
#include <vector>

#include "export_indexed_triangles.h"

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "Reindexer.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"


std::pair<std::vector<Vector3d>, std::vector<IndexedTriangle>>
export_indexed_triangles(const PolySet &ps)
{
	PolySet triangulated(3);
	PolysetUtils::tessellate_faces(ps, triangulated);

	Reindexer<Vector3d> reindexer;
	std::vector<Vector3d> vecs;
	std::vector<IndexedTriangle> tris;

	for(const auto &p : triangulated.polygons) {
		assert(p.size() == 3);

		Vector3d vec1 = {p[0][0], p[0][1], p[0][2]};
		Vector3d vec2 = {p[1][0], p[1][1], p[1][2]};
		Vector3d vec3 = {p[2][0], p[2][1], p[2][2]};

		int i1 = reindexer.lookup(vec1);
		int i2 = reindexer.lookup(vec2);
		int i3 = reindexer.lookup(vec3);

		tris.push_back(IndexedTriangle({i1, i2, i3}));
	}

	vecs.resize(reindexer.size());
	reindexer.copy(vecs.begin());

	return std::make_pair(vecs, tris);
}

std::pair<std::vector<Vector3d>, std::vector<IndexedTriangle>>
export_indexed_triangles(const CGAL_Polyhedron &P)
{
	typedef CGAL_Polyhedron::Vertex																	Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator									VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator										FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	Reindexer<Vector3d> reindexer;
	std::vector<Vector3d> vecs;
	std::vector<IndexedTriangle> tris;

	for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
		HFCC hc = fi->facet_begin();
		HFCC hc_end = hc;
		Vertex v1, v2, v3;
		v1 = *VCI((hc++)->vertex());
		v3 = *VCI((hc++)->vertex());
		do {
			v2 = v3;
			v3 = *VCI((hc++)->vertex());
			double x1 = CGAL::to_double(v1.point().x());
			double y1 = CGAL::to_double(v1.point().y());
			double z1 = CGAL::to_double(v1.point().z());
			double x2 = CGAL::to_double(v2.point().x());
			double y2 = CGAL::to_double(v2.point().y());
			double z2 = CGAL::to_double(v2.point().z());
			double x3 = CGAL::to_double(v3.point().x());
			double y3 = CGAL::to_double(v3.point().y());
			double z3 = CGAL::to_double(v3.point().z());

			Vector3d vec1 = {x1, y1, z1};
			Vector3d vec2 = {x2, y2, z2};
			Vector3d vec3 = {x3, y3, z3};

			int i1 = reindexer.lookup(vec1);
			int i2 = reindexer.lookup(vec2);
			int i3 = reindexer.lookup(vec3);

			tris.push_back(IndexedTriangle({i1, i2, i3}));
		} while (hc != hc_end);
	}

	vecs.resize(reindexer.size());
	reindexer.copy(vecs.begin());

	return std::make_pair(vecs, tris);
}

#endif // ENABLE_CGAL
