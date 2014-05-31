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
	if (OSColors::colorschemes.count("Cornfield")>0)
		return OSColors::colorschemes["Cornfield"];
	else
		return OSColors::defaultColorScheme;
}

Color4f RenderSettings::color(OSColors::RenderColor idx)
{
	return this->colors[idx];
}

void RenderSettings::setColors(const OSColors::colorscheme &colors)
{
	this->colors = colors;
}

OSColors::colorscheme &RenderSettings::getColors()
{
	return this->colors;
}
