#include "imageutils.h"
#include "lodepng.h"
#include <stdio.h>
#include <stdlib.h>

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
  //encoder.settings.zlibsettings.windowSize = 2048;
  //LodePNG_Text_add(&encoder.infoPng.text, "Comment", "Created with LodePNG");
  size_t dataout_size = -1;
	bool result = false;
  unsigned char *dataout = (unsigned char *)malloc(width*height*4);
	if (!dataout) {
		perror("Error allocating memory while writing png\n");
		return result;
	}
  LodePNG_encode(&dataout, &dataout_size, pixels, width, height, LCT_RGBA, 8);
	try {
		output.write( reinterpret_cast<const char*>(dataout), dataout_size );
		result = true;
	} catch (const std::ios_base::failure &e) {
    std::cerr << "Error writing to ostream:" << e.what() << "\n";
	}
	free( dataout );
  return result;
}
