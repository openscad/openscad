#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <stdio.h>
#include "polyset.h"

#ifdef ENABLE_CGAL
#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "cgal.h"

void export_png_with_cgal(CGAL_Nef_polyhedron *root_N, std::ostream &output)
{
	CsgInfo csgInfo;
	try {
		csgInfo.glview = new OffscreenView(512,512);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
	}
	CGALRenderer cgalRenderer(*root_N);

	BoundingBox bbox;
	if (cgalRenderer.polyhedron) {
		CGAL::Bbox_3 cgalbbox = cgalRenderer.polyhedron->bbox();
		bbox = BoundingBox(
		  Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
		  Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax())  );
	}
	else if (cgalRenderer.polyset) {
		bbox = cgalRenderer.polyset->getBoundingBox();
	}

	Vector3d center = getBoundingCenter(bbox);
	double radius = getBoundingRadius(bbox);

	Vector3d cameradir(1, 1, -0.5);
	Vector3d camerapos = center - radius*2*cameradir;
	output << center << "\n";
	output << radius << "\n";
/*
	csgInfo.glview->setCamera(camerapos, center);
	csgInfo.glview->setRenderer(&cgalRenderer);
        csgInfo.glview->paintGL();
        csgInfo.glview->save(outfile);
*/
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
