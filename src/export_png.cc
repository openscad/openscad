#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <stdio.h>

#ifdef ENABLE_CGAL

void export_png_with_cgal(CGAL_Nef_polyhedron *root_N, std::ostream &output)
{
	CsgInfo csgInfo;
	try {
		csgInfo.glview = new OffscreenView(512,512);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
	}
	output << "solid OpenSCAD_Model\n";
	output << "endsolid OpenSCAD_Model\n";
}

void export_png_with_opencsg(CGAL_Nef_polyhedron *root_N, std::ostream &output)
{
	CsgInfo csgInfo;
	output << "solid OpenSCAD_Model opencsg\n";
	output << "endsolid OpenSCAD_Model opencsg\n";
}


#endif // ENABLE_CGAL
