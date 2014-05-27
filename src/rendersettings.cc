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
	setColors( defaultColorScheme() );
}

OSColors::colorscheme &RenderSettings::defaultColorScheme()
{
	return OSColors::schemes["Cornfield"];
}

Color4f RenderSettings::color(OSColors::RenderColors::RenderColor idx)
{
	return this->colors[idx];
}

void RenderSettings::setColors(const OSColors::colorscheme &colors)
{
	PRINT("RS: setcolors");
	this->colors = colors;
}

OSColors::colorscheme &RenderSettings::getColors()
{
	return this->colors;
}

