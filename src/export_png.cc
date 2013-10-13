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
	try {
		glview = new OffscreenView( cam.pixel_width, cam.pixel_height );
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
		cam.type = Camera::VECTOR;
		cam.center = getBoundingCenter(bbox);
		double radius = getBoundingRadius(bbox);
		Vector3d cameradir(1, 1, -0.5);
		cam.eye = cam.center - radius*2*cameradir;
	}

	glview->setCamera( cam );
	glview->setRenderer(&cgalRenderer);
	glview->paintGL();
	glview->save(output);
}

enum Previewer { OPENCSG, THROWN } previewer;

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"

void export_png_preview_common( Tree &tree, Camera &cam, std::ostream &output, Previewer previewer = OPENCSG )
{
	CsgInfo csgInfo = CsgInfo();
	if ( !csgInfo.compile_chains( tree ) ) {
		fprintf(stderr,"Couldn't initialize CSG chains\n");
		return;
	}

	try {
		csgInfo.glview = new OffscreenView( cam.pixel_width, cam.pixel_height );
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
	}

#ifdef ENABLE_OPENCSG
	OpenCSGRenderer openCSGRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);
#endif
	ThrownTogetherRenderer thrownTogetherRenderer( csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain );

	if (cam.type == Camera::NONE) {
		cam.type = Camera::VECTOR;
		double radius = 1.0;
		if (csgInfo.root_chain) {
			BoundingBox bbox = csgInfo.root_chain->getBoundingBox();
			cam.center = (bbox.min() + bbox.max()) / 2;
			radius = (bbox.max() - bbox.min()).norm() / 2;
		}
		Vector3d cameradir(1, 1, -0.5);
		cam.eye = cam.center - radius*1.8*cameradir;
	}

	csgInfo.glview->setCamera( cam );
	if ( previewer == OPENCSG )
		csgInfo.glview->setRenderer( &openCSGRenderer );
	else
		csgInfo.glview->setRenderer( &thrownTogetherRenderer );
#ifdef ENABLE_OPENCSG
	OpenCSG::setContext( 0 );
	OpenCSG::setOption( OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject );
#endif
	csgInfo.glview->paintGL();
	csgInfo.glview->save( output );
}

void export_png_with_opencsg(Tree &tree, Camera &cam, std::ostream &output)
{
#ifdef ENABLE_OPENCSG
	export_png_preview_common( tree, cam, output, OPENCSG );
#else
	fprintf(stderr,"This openscad was built without OpenCSG support\n");
#endif
}

void export_png_with_throwntogether(Tree &tree, Camera &cam, std::ostream &output)
{
	export_png_preview_common( tree, cam, output, THROWN );
}


#endif // ENABLE_CGAL
