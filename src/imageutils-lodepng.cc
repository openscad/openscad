#include "imageutils.h"
#include "lodepng.h"
#include <stdio.h>
#include <stdlib.h>

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
  //encoder.settings.zlibsettings.windowSize = 2048;
  //LodePNG_Text_add(&encoder.infoPng.text, "Comment", "Created with LodePNG");
  size_t dataout_size = -1;
  unsigned char *dataout = (unsigned char *)malloc(width*height*4);
  LodePNG_encode(&dataout, &dataout_size, pixels, width, height, LCT_RGBA, 8);
	output.write( reinterpret_cast<const char*>(dataout), dataout_size );;
	free( dataout );
  return true;
}
