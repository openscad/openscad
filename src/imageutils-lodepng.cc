#include "lodepng.h"
#include <stdio.h>
#include <stdlib.h>

bool write_png(const char *filename, unsigned char *pixels, int width, int height)
{
  //encoder.settings.zlibsettings.windowSize = 2048;
  //LodePNG_Text_add(&encoder.infoPng.text, "Comment", "Created with LodePNG");
  
  size_t dataout_size = -1;
  unsigned char *dataout = (unsigned char *)malloc(width*height*4);
  LodePNG_encode(&dataout, &dataout_size, pixels, width, height, LCT_RGBA, 8);
  //LodePNG_saveFile(dataout, dataout_size, "blah2.png");

  FILE *f = fopen(filename, "wb");
  if (!f) {
		free(dataout);
		return false;
	}

	fwrite(dataout, 1, dataout_size, f);
	fclose(f);
  free(dataout);
  return true;
}
