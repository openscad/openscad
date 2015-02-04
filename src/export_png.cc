#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <stdio.h>
#include "polyset.h"
#include "rendersettings.h"

#ifdef ENABLE_CGAL
#include "CGALRenderer.h"
#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"

static void setupCamera(Camera &cam, const BoundingBox &bbox, float scalefactor)
{
	PRINTDB("setupCamera() %i",cam.type);
	if (cam.type == Camera::NONE) cam.viewall = true;
	if (cam.viewall) cam.viewAll(bbox, scalefactor);
}

void export_png(shared_ptr<const Geometry> root_geom, Camera &cam, std::ostream &output)
{
	PRINTD("export_png geom");
	OffscreenView *glview;
	try {
		glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
	}
	CGALRenderer cgalRenderer(root_geom);

	BoundingBox bbox = cgalRenderer.getBoundingBox();
	setupCamera(cam, bbox, 3);

	glview->setCamera(cam);
	glview->setRenderer(&cgalRenderer);
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->paintGL();
	glview->save(output);
}

enum Previewer { OPENCSG, THROWNTOGETHER } previewer;

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"

void export_png_preview_common(Tree &tree, Camera &cam, std::ostream &output, Previewer previewer = OPENCSG)
{
	PRINTD("export_png_preview_common");
	CsgInfo csgInfo = CsgInfo();
    csgInfo.compile_chains(tree);

	try {
		csgInfo.glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
	}

#ifdef ENABLE_OPENCSG
	OpenCSGRenderer openCSGRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);
#endif
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain);

#ifdef ENABLE_OPENCSG
	if (previewer == OPENCSG)
		csgInfo.glview->setRenderer(&openCSGRenderer);
	else
#endif
		csgInfo.glview->setRenderer(&thrownTogetherRenderer);
#ifdef ENABLE_OPENCSG
	BoundingBox bbox = csgInfo.glview->getRenderer()->getBoundingBox();
	setupCamera(cam, bbox, 2.7);

	csgInfo.glview->setCamera(cam);
	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
#endif
	csgInfo.glview->setColorScheme(RenderSettings::inst()->colorscheme);
	csgInfo.glview->paintGL();
	csgInfo.glview->save(output);
}

void export_png_with_opencsg(Tree &tree, Camera &cam, std::ostream &output)
{
	PRINTD("export_png_w_opencsg");
#ifdef ENABLE_OPENCSG
	export_png_preview_common(tree, cam, output, OPENCSG);
#else
	fprintf(stderr,"This openscad was built without OpenCSG support\n");
#endif
}

void export_png_with_throwntogether(Tree &tree, Camera &cam, std::ostream &output)
{
	PRINTD("export_png_w_thrown");
	export_png_preview_common(tree, cam, output, THROWNTOGETHER);
}

#endif // ENABLE_CGAL
