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

static void setupCamera(Camera &cam, const BoundingBox &bbox)
{
	PRINTDB("setupCamera() %i",cam.type);
	if (cam.type == Camera::NONE) cam.viewall = true;
	if (cam.viewall) cam.viewAll(bbox);
}

bool export_png(const shared_ptr<const Geometry> &root_geom, Camera &cam, std::ostream &output)
{
	PRINTD("export_png geom");
	OffscreenView *glview;
	try {
		glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return false;
	}
	CGALRenderer cgalRenderer(root_geom);

	BoundingBox bbox = cgalRenderer.getBoundingBox();
	setupCamera(cam, bbox);

	glview->setCamera(cam);
	glview->setRenderer(&cgalRenderer);
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->paintGL();
	glview->save(output);
	return true;
}

enum Previewer { OPENCSG, THROWNTOGETHER } previewer;

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"

bool export_png_preview_common(Tree &tree, Camera &cam, std::ostream &output, Previewer previewer = OPENCSG)
{
	PRINTD("export_png_preview_common");
	CsgInfo csgInfo = CsgInfo();
	csgInfo.compile_products(tree);

	OffscreenView *glview;
	try {
		glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return false;
	}

#ifdef ENABLE_OPENCSG
	OpenCSGRenderer openCSGRenderer(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products, glview->shaderinfo);
#endif
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);

#ifdef ENABLE_OPENCSG
	if (previewer == OPENCSG)
		glview->setRenderer(&openCSGRenderer);
	else
#endif
		glview->setRenderer(&thrownTogetherRenderer);
#ifdef ENABLE_OPENCSG
	BoundingBox bbox = glview->getRenderer()->getBoundingBox();
	setupCamera(cam, bbox);

	glview->setCamera(cam);
	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
#endif
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->paintGL();
	glview->save(output);
	return true;
}

bool export_png_with_opencsg(Tree &tree, Camera &cam, std::ostream &output)
{
	PRINTD("export_png_w_opencsg");
#ifdef ENABLE_OPENCSG
	return export_png_preview_common(tree, cam, output, OPENCSG);
#else
	fprintf(stderr,"This openscad was built without OpenCSG support\n");
	return false;
#endif
}

bool export_png_with_throwntogether(Tree &tree, Camera &cam, std::ostream &output)
{
	PRINTD("export_png_w_thrown");
	return export_png_preview_common(tree, cam, output, THROWNTOGETHER);
}

#endif // ENABLE_CGAL
