#include "imageutils.h"
#include "lodepng.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <algorithm>
#include "rendersettings.h"

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
	std::vector<unsigned char> dataout;
	lodepng::State state;
	state.info_png.background_defined = true;
	Color4f bg = RenderSettings::inst()->color(RenderSettings::BACKGROUND_COLOR);
	state.info_png.background_r = bg(0);
	state.info_png.background_g = bg(1);
	state.info_png.background_b = bg(2);
	state.info_png.color.colortype = LCT_RGBA;
	state.info_png.color.bitdepth = 8;
	unsigned err = lodepng::encode(dataout, pixels, width, height, state);
	if ( err ) return false;
	output.write( reinterpret_cast<const char *>(&dataout[0]), dataout.size());
	if ( output.bad() ) std::cerr << "Error writing to ostream\n";
	return output.good();
}
