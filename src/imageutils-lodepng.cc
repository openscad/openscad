#include "imageutils.h"
#include "lodepng.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <algorithm>

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
	std::vector<unsigned char> dataout;
	unsigned err = lodepng::encode(dataout, pixels, width, height, LCT_RGBA, 8);
	if ( err ) return false;
	output.write( reinterpret_cast<const char *>(&dataout[0]), dataout.size());
	if ( output.bad() ) std::cerr << "Error writing to ostream\n";
	return output.good();
}
