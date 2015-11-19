#include "imageutils.h"
#include "ext/lodepng/lodepng.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <algorithm>
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
	std::vector<unsigned char> dataout;
	lodepng::State state;
	state.encoder.auto_convert = false;

	// Prior to 1.4.0, OpenCSG uses Alpha channel for visibility computations
	#if defined(ENABLE_OPENCSG) && OPENCSG_VERSION >= 0x0140
	state.info_png.color.colortype = LCT_RGBA;
	#else
	state.info_png.color.colortype = LCT_RGB;
	#endif

	state.info_png.color.bitdepth = 8;
	unsigned err = lodepng::encode(dataout, pixels, width, height, state);
	if ( err ) return false;
	output.write( reinterpret_cast<const char *>(&dataout[0]), dataout.size());
	if ( output.bad() ) std::cerr << "Error writing to ostream\n";
	return output.good();
}
