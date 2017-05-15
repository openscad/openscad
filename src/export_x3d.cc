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

#include <ostream>

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"

#include "export_indexed_triangles.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

static void append_x3d(const std::vector<Vector3d> &vecs, const std::vector<IndexedTriangle> &tris, std::ostream &output)
{
	output << "    <Shape>\r\n"
				 << "      <Appearance>\r\n"
				 << "        <Material diffuseColor='0.5 0.5 0.5'>\r\n"
				 << "        </Material>\r\n"
				 << "      </Appearance>\r\n"
				 << "      <IndexedTriangleSet index='";

	for (const auto &tri: tris)
		output << tri(0) << " " << tri(1) << " " << tri(2) << " ";

	output << "'>\r\n"
				 << "        <Coordinate point='";

	for (const auto &v: vecs)
		output << v(0) << " " << v(1) << " " << v(2) << ", ";

	output << "'>\r\n"
				 << "        </Coordinate>\r\n"
				 << "      </IndexedTriangleSet>\r\n"
				 << "    </Shape>\r\n";
}

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
static void append_x3d(const CGAL_Nef_polyhedron &root_N, std::ostream &output)
{
	if (!root_N.p3->is_simple()) {
		PRINT("WARNING: Exported object may not be a valid 2-manifold and may need repair");
	}

	bool usePolySet = true;
	if (usePolySet) {
		PolySet ps(3);
		bool err = CGALUtils::createPolySetFromNefPolyhedron3(*(root_N.p3), ps);
		if (err) { PRINT("ERROR: Nef->PolySet failed"); }
		else {
			auto r = export_indexed_triangles(ps);
			append_x3d(r.first, r.second, output);
		}
	}
	else {
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			CGAL_Polyhedron P;
			//root_N.p3->convert_to_Polyhedron(P);
			bool err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>( *(root_N.p3), P );
			if (err) {
				PRINT("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed");
				return;
			}
			auto r = export_indexed_triangles(P);
			append_x3d(r.first, r.second, output);
		}
		catch (const CGAL::Assertion_exception &e) {
			PRINTB("ERROR: CGAL error in CGAL_Nef_polyhedron3::convert_to_Polyhedron(): %s", e.what());
		}
		catch (...) {
			PRINT("ERROR: CGAL unknown error in CGAL_Nef_polyhedron3::convert_to_Polyhedron()");
		}
		CGAL::set_error_behaviour(old_behaviour);
	}
}

static void append_x3d(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(geom.get())) {
		append_x3d(*N, output);
	}
	else if (const PolySet *ps = dynamic_cast<const PolySet *>(geom.get())) {
		auto r = export_indexed_triangles(*ps);
		append_x3d(r.first, r.second, output);
	}
	// else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
	// 	assert(false && "Unsupported file format");
	// }
	else {
		assert(false && "Not implemented");
	}
}

void export_x3d(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

	output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
				 << "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 3.0//EN\" "
				 <<   "\"http://www.web3d.org/specifications/x3d-3.0.dtd\">\r\n"
				 << "<X3D version=\"3.0\" profile=\"Immersive\" "
				 <<   "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema-instance\" "
				 <<   "xsd:noNamespaceSchemaLocation=\"http://www.web3d.org/specifications/x3d-3.0.xsd\">\r\n"
				 << "  <head>\r\n"
				 << "    <meta name=\"generator\" content=\"OpenSCAD " << QUOTED(OPENSCAD_VERSION) << "\" />\r\n"
				 << "  </head>\r\n"
				 << "  <Scene>\r\n";

	append_x3d(geom, output);

	output << "  </Scene>\r\n"
				 << "</X3D>\r\n";

	setlocale(LC_NUMERIC, "");			// Set default locale
}

#endif // ENABLE_CGAL
