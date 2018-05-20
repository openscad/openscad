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
#include "dxfdata.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

#define OSS(X) static_cast<std::ostringstream &&>(std::ostringstream() << X).str()

namespace {

std::string toString(const Vector3d &v)
{
	return OSS(v[0] << " " << v[1] << " " << v[2]);
}

Vector3d fromString(const std::string &vertexString)
{
	Vector3d v;
	std::istringstream stream{vertexString};
	stream >> v[0] >> v[1] >> v[2];
	return v;
}
	
void append_stl(const PolySet &ps, std::ostream &output)
{
	PolySet triangulated(3);
	PolysetUtils::tessellate_faces(ps, triangulated);

	for(const auto &p : triangulated.polygons) {
		assert(p.size() == 3); // STL only allows triangles
		std::array<std::string, 3> vertexStrings;
		std::transform(p.cbegin(), p.cend(), vertexStrings.begin(), toString);

		if (vertexStrings[0] != vertexStrings[1] &&
				vertexStrings[0] != vertexStrings[2] &&
				vertexStrings[1] != vertexStrings[2]) {
			// The above condition ensures that there are 3 distinct vertices, but
			// they may be collinear. If they are, the unit normal is meaningless
			// so the default value of "0 0 0" can be used. If the vertices are not
			// collinear then the unit normal must be calculated from the
			// components.
			output << "  facet normal ";

			Vector3d p0 = fromString(vertexStrings[0]);
			Vector3d p1 = fromString(vertexStrings[1]);
			Vector3d p2 = fromString(vertexStrings[2]);
			
			Vector3d normal = (p1 - p0).cross(p2 - p0);
			normal.normalize();
			if (is_finite(normal) && !is_nan(normal)) {
				output << normal[0] << " " << normal[1] << " " << normal[2] << "\n";
			}
			else {
				output << "0 0 0\n";
			}
			output << "    outer loop\n";
		
			for (const auto &vertexString : vertexStrings) {
				output << "      vertex " << vertexString << "\n";
			}
			output << "    endloop\n";
			output << "  endfacet\n";
		}
	}
}

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
void append_stl(const CGAL_Nef_polyhedron &root_N, std::ostream &output)
{
	if (!root_N.p3->is_simple()) {
		PRINT("WARNING: Exported object may not be a valid 2-manifold and may need repair");
	}

	PolySet ps(3);
	if (!CGALUtils::createPolySetFromNefPolyhedron3(*(root_N.p3), ps)) {
		append_stl(ps, output);
	}
	else {
		PRINT("ERROR: Nef->PolySet failed");
	}
}

void append_stl(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(geom.get())) {
		append_stl(*N, output);
	}
	else if (const PolySet *ps = dynamic_cast<const PolySet *>(geom.get())) {
		append_stl(*ps, output);
	}
	else if (dynamic_cast<const Polygon2d *>(geom.get())) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}
}

} // namespace

void export_stl(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	output << "solid OpenSCAD_Model\n";

	append_stl(geom, output);

	output << "endsolid OpenSCAD_Model\n";
	setlocale(LC_NUMERIC, "");      // Set default locale
}

#endif // ENABLE_CGAL
