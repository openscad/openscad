#include "imageutils.h"
#include <strings.h>

void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width, size_t height)
{
	size_t rowBytes = pixelsize * width;
	for (size_t i = 0 ; i < height ; i++) {
    bcopy(src + i * rowBytes, dst + (height - i - 1) * rowBytes, rowBytes);
  }
}

#ifdef __APPLE__
#include "imageutils-macosx.cc"
#else
#include "imageutils-lodepng.cc"
#endif
