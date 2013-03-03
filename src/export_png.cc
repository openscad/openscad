#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <stdio.h>
#include "polyset.h"
#include "rendersettings.h"

#ifdef ENABLE_CGAL
#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "cgal.h"

void export_png_with_cgal(CGAL_Nef_polyhedron *root_N, Camera &cam, std::ostream &output)
{
	OffscreenView *glview;
	int w = RenderSettings::inst()->img_width;
	int h = RenderSettings::inst()->img_height;
	try {
		glview = new OffscreenView( w, h );
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
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

	if (cam.type == Camera::NONE) {
		VectorCamera vcam;
		vcam.center = getBoundingCenter(bbox);
		double radius = getBoundingRadius(bbox);
		Vector3d cameradir(1, 1, -0.5);
		vcam.eye = vcam.center - radius*2*cameradir;
		cam.set(vcam);
	}

	//std::cerr << center << "\n";
	//std::cerr << radius << "\n";

	glview->setCamera( cam );
	glview->setRenderer(&cgalRenderer);
	glview->paintGL();
	glview->save(output);
}

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif

void export_png_with_opencsg(Tree &tree, Camera &cam, std::ostream &output)
{
#ifdef ENABLE_OPENCSG
  CsgInfo csgInfo = CsgInfo();
  if ( !csgInfo.prep_chains( tree ) ) {
		fprintf(stderr,"Couldn't initialize OpenCSG chains\n");
		return;
	}

	int w = RenderSettings::inst()->img_width;
	int h = RenderSettings::inst()->img_height;
	try {
		csgInfo.glview = new OffscreenView( w, h );
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
	}

	OpenCSGRenderer opencsgRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);

	if (cam.type == Camera::NONE) {
		VectorCamera vcam;
		vcam.center << 0,0,0;
	  double radius = 1.0;
	  if (csgInfo.root_chain) {
	    BoundingBox bbox = csgInfo.root_chain->getBoundingBox();
	    vcam.center = (bbox.min() + bbox.max()) / 2;
	    radius = (bbox.max() - bbox.min()).norm() / 2;
	  }
	  Vector3d cameradir(1, 1, -0.5);
	  vcam.eye = vcam.center - radius*1.8*cameradir;
		cam.set(vcam);
	}

	csgInfo.glview->setCamera( cam );
  csgInfo.glview->setRenderer(&opencsgRenderer);
  OpenCSG::setContext(0);
  OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
  csgInfo.glview->paintGL();
  csgInfo.glview->save(output);
#else
	fprintf(stderr,"This openscad was built without OpenCSG support\n");
#endif
}


#endif // ENABLE_CGAL
