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
	if (cam.viewall) cam.viewAll(bbox);
}

bool export_png(const shared_ptr<const Geometry> &root_geom, const ViewOptions& options, Camera camera, std::ostream &output)
{
	PRINTD("export_png geom");
	OffscreenView *glview;
	try {
		glview = new OffscreenView(camera.pixel_width, camera.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return false;
	}
	CGALRenderer cgalRenderer(root_geom);

	BoundingBox bbox = cgalRenderer.getBoundingBox();
	setupCamera(camera, bbox);

	glview->setCamera(camera);
	glview->setRenderer(&cgalRenderer);
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->setShowFaces(!options["wireframe"]);
	glview->setShowCrosshairs(options["crosshairs"]);
	glview->setShowAxes(options["axes"]);
	glview->setShowScaleProportional(options["scales"]);
	glview->setShowEdges(options["edges"]);
	glview->paintGL();
	glview->save(output);
	return true;
}

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"

std::unique_ptr<OffscreenView> prepare_preview(Tree &tree, const ViewOptions& options, Camera camera)
{
	PRINTD("prepare_preview_common");
	CsgInfo csgInfo = CsgInfo();
	csgInfo.compile_products(tree);

	std::unique_ptr<OffscreenView> glview;
	try {
		glview.reset(new OffscreenView(camera.pixel_width, camera.pixel_height));
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return 0;
	}

#ifdef ENABLE_OPENCSG
	OpenCSGRenderer openCSGRenderer(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
#endif
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);

	if (options.previewer == Previewer::OPENCSG) {
#ifdef ENABLE_OPENCSG
		glview->setRenderer(&openCSGRenderer);
#else
		fprintf(stderr,"This openscad was built without OpenCSG support\n");
		return 0;
#endif
	}
	else {
		glview->setRenderer(&thrownTogetherRenderer);
	}
#ifdef ENABLE_OPENCSG
	BoundingBox bbox = glview->getRenderer()->getBoundingBox();
	setupCamera(camera, bbox);

	glview->setCamera(camera);
	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
#endif
	glview->setColorScheme(RenderSettings::inst()->colorscheme);
	glview->setShowAxes(options["axes"]);
	glview->setShowScaleProportional(options["scales"]);
	glview->setShowEdges(options["edges"]);
	glview->paintGL();
	return glview;
}

bool export_png(const OffscreenView &glview, std::ostream &output)
{
	PRINTD("export_png_preview_common");
	glview.save(output);
	return true;
}

#endif // ENABLE_CGAL
