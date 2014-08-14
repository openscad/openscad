#include "rendersettings.h"
#include "colormap.h"
#include "printutils.h"

RenderSettings *RenderSettings::inst(bool erase)
{
	static RenderSettings *instance = new RenderSettings;
	if (erase) {
		delete instance;
		instance = NULL;
	}
	return instance;
}

RenderSettings::RenderSettings()
{
	openCSGTermLimit = 100000;
	far_gl_clip_limit = 100000.0;
	img_width = 512;
	img_height = 512;
	colorscheme = "Cornfield";
}
