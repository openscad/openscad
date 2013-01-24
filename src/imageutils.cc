#include "imageutils.h"
#include <string.h>

void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width, size_t height)
{
	size_t rowBytes = pixelsize * width;
	for (size_t i = 0 ; i < height ; i++) {
    memmove(dst + (height - i - 1) * rowBytes, src + i * rowBytes, rowBytes);
  }
}
