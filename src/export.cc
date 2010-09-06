/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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
#include "polyset.h"
#include "dxfdata.h"

#include <QApplication>
#include <QProgressDialog>
#include <QTextStream>
#include <errno.h>

#ifdef ENABLE_CGAL
#include "cgal.h"

void cgal_nef3_to_polyset(PolySet *ps, CGAL_Nef_polyhedron *root_N)
{
	CGAL_Polyhedron P;
	root_N->p3.convert_to_Polyhedron(P);

	typedef CGAL_Polyhedron::Vertex                                 Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator                  VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator                   FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

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
			ps->append_poly();
			ps->append_vertex(x1, y1, z1);
			ps->append_vertex(x2, y2, z2);
			ps->append_vertex(x3, y3, z3);
		} while (hc != hc_end);
	}
}

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
void export_stl(CGAL_Nef_polyhedron *root_N, QTextStream &output, QProgressDialog *pd)
{
	CGAL_Polyhedron P;
	root_N->p3.convert_to_Polyhedron(P);

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
			QString vs1, vs2, vs3;
			vs1.sprintf("%f %f %f", x1, y1, z1);
			vs2.sprintf("%f %f %f", x2, y2, z2);
			vs3.sprintf("%f %f %f", x3, y3, z3);
			if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
				
				double nx = (y1-y2)*(z1-z3) - (z1-z2)*(y1-y3);
				double ny = (z1-z2)*(x1-x3) - (x1-x2)*(z1-z3);
				double nz = (x1-x2)*(y1-y3) - (y1-y2)*(x1-x3);
				double nlength = sqrt(nx*nx + ny*ny + nz*nz);
				// Avoid generating normals for polygons with zero area
				double eps = 0.000001;
				if (nlength < eps) nlength = 1.0;
				output << "  facet normal " 
							 << nx / nlength << " " 
							 << ny / nlength << " " 
							 << nz / nlength << "\n";
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

void export_off(CGAL_Nef_polyhedron*, QTextStream&, QProgressDialog*)
{
	PRINTF("WARNING: OFF import is not implemented yet.");
}

/*!
	Saves the current 2D CGAL Nef polyhedron as DXF to the given absolute filename.
 */
void export_dxf(CGAL_Nef_polyhedron *root_N, QTextStream &output, QProgressDialog *)
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

	DxfData dd(*root_N);
	for (int i=0; i<dd.paths.size(); i++)
	{
		for (int j=1; j<dd.paths[i].points.size(); j++) {
			DxfData::Point *p1 = dd.paths[i].points[j-1];
			DxfData::Point *p2 = dd.paths[i].points[j];
			double x1 = p1->x;
			double y1 = p1->y;
			double x2 = p2->x;
			double y2 = p2->y;
			output << "  0\n"
						 << "LINE\n";
			// Some importers (e.g. Inkscape) needs a layer to be specified
			output <<"  8\n"
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

	setlocale(LC_NUMERIC, "");      // Set default locale
}

#endif

