/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"
#include "printutils.h"
#include "MainWindow.h"

#include <QApplication>

#ifdef ENABLE_CGAL

void export_stl(CGAL_Nef_polyhedron *root_N, QString filename, QProgressDialog *pd)
{
	CGAL_Polyhedron P;
	root_N->convert_to_Polyhedron(P);

	typedef CGAL_Polyhedron::Vertex                                 Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator                  VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator                   FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	FILE *f = fopen(filename.toUtf8().data(), "w");
	if (!f) {
		PRINTA("Can't open STL file \"%1\" for STL export: %2", 
					 filename, QString(strerror(errno)));
		MainWindow::current_win = NULL;
		return;
	}
	fprintf(f, "solid\n");

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
				double n_scale = 1 / sqrt(nx*nx + ny*ny + nz*nz);
				fprintf(f, "  facet normal %f %f %f\n",
						nx * n_scale, ny * n_scale, nz * n_scale);
				fprintf(f, "    outer loop\n");
				fprintf(f, "      vertex %s\n", vs1.toAscii().data());
				fprintf(f, "      vertex %s\n", vs2.toAscii().data());
				fprintf(f, "      vertex %s\n", vs3.toAscii().data());
				fprintf(f, "    endloop\n");
				fprintf(f, "  endfacet\n");
			}
		} while (hc != hc_end);
		if (pd) {
			pd->setValue(facet_count++);
			QApplication::processEvents();
		}
	}

	fprintf(f, "endsolid\n");
	fclose(f);
}

void export_off(CGAL_Nef_polyhedron*, QString, QProgressDialog*)
{
	PRINTF("WARNING: OFF import is not implemented yet.");
}

#endif

