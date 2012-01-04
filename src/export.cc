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
#include "printutils.h"
#include "polyset.h"
#include "dxfdata.h"

#include <QApplication>
#include <QProgressDialog>
#include <errno.h>

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
void export_stl(CGAL_Nef_polyhedron *root_N, std::ostream &output, QProgressDialog *pd)
{
	CGAL_Polyhedron P;
	root_N->p3->convert_to_Polyhedron(P);

	typedef CGAL_Polyhedron::Vertex                                 Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator                  VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator                   FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

	output << "solid OpenSCAD_Model\n";

	int facet_count = 0;
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
			std::stringstream stream;
			stream << x1 << " " << y1 << " " << z1;
			std::string vs1 = stream.str();
			stream.str("");
			stream << x2 << " " << y2 << " " << z2;
			std::string vs2 = stream.str();
			stream.str("");
			stream << x3 << " " << y3 << " " << z3;
			std::string vs3 = stream.str();
			CGAL_Polyhedron::Traits::Vector_3 normal(1,0,0);
			if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
				// The above condition ensures that there are 3 distinct vertices, but
				// they may be collinear. If they are, the unit normal is meaningless
				// so the default value of "1 0 0" can be used. If the vertices are not
				// collinear then the unit normal must be calculated from the
				// components.
				if (!CGAL::collinear(v1.point(),v2.point(),v3.point())) {
					// Pseudocode: CGAL kernel must be set up to enable unit_normal and
					// Vector type must be declared as Vector_3<Kernel>.
					// http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Kernel_23_ref/Function_unit_normal.html
					normal = CGAL::normal(v1.point(),v2.point(),v3.point());
					normal = normal / sqrt(CGAL::to_double(normal.squared_length()));
				}	
				output << "  facet normal "
							 << CGAL::to_double(normal.x()) << " "
							 << CGAL::to_double(normal.y()) << " "
							 << CGAL::to_double(normal.z()) << "\n";
				output << "    outer loop\n";
				output << "      vertex " << vs1 << "\n";
				output << "      vertex " << vs2 << "\n";
				output << "      vertex " << vs3 << "\n";
				output << "    endloop\n";
				output << "  endfacet\n";
			}
		} while (hc != hc_end);
		if (pd) {
			pd->setValue(facet_count++);
			QApplication::processEvents();
		}
	}

	output << "endsolid OpenSCAD_Model\n";
	setlocale(LC_NUMERIC, "");      // Set default locale
}

void export_off(CGAL_Nef_polyhedron *root_N, std::ostream &output, QProgressDialog*)
{
	CGAL_Polyhedron P;
	root_N->p3->convert_to_Polyhedron(P);
	output << P;
}

/*!
	Saves the current 2D CGAL Nef polyhedron as DXF to the given absolute filename.
 */
void export_dxf(CGAL_Nef_polyhedron *root_N, std::ostream &output, QProgressDialog *)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	// Some importers (e.g. Inkscape) needs a BLOCKS section to be present
	output << "  0\n"
				 <<	"SECTION\n"
				 <<	"  2\n"
				 <<	"BLOCKS\n"
				 <<	"  0\n"
				 << "ENDSEC\n"
				 << "  0\n"
				 << "SECTION\n"
				 << "  2\n"
				 << "ENTITIES\n";

	DxfData *dd =root_N->convertToDxfData();
	for (size_t i=0; i<dd->paths.size(); i++)
	{
		for (size_t j=1; j<dd->paths[i].indices.size(); j++) {
			const Vector2d &p1 = dd->points[dd->paths[i].indices[j-1]];
			const Vector2d &p2 = dd->points[dd->paths[i].indices[j]];
			double x1 = p1[0];
			double y1 = p1[1];
			double x2 = p2[0];
			double y2 = p2[1];
			output << "  0\n"
						 << "LINE\n";
			// Some importers (e.g. Inkscape) needs a layer to be specified
			output << "  8\n"
						 << "0\n"
						 << " 10\n"
						 << x1 << "\n"
						 << " 11\n"
						 << x2 << "\n"
						 << " 20\n"
						 << y1 << "\n"
						 << " 21\n"
						 << y2 << "\n";
		}
	}

	output << "  0\n"
				 << "ENDSEC\n";

	// Some importers (e.g. Inkscape) needs an OBJECTS section with a DICTIONARY entry
	output << "  0\n"
				 << "SECTION\n"
				 << "  2\n"
				 << "OBJECTS\n"
				 << "  0\n"
				 << "DICTIONARY\n"
				 << "  0\n"
				 << "ENDSEC\n";

	output << "  0\n"
				 <<"EOF\n";

	delete dd;
	setlocale(LC_NUMERIC, "");      // Set default locale
}

#endif

