#include "imageutils.h"
#include <assert.h>
#include <string.h>
#include <fstream>

void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width, size_t height)
{
  assert(src && dst);
  auto rowBytes = pixelsize * width;
  for (auto i = 0ul;i < height;i++) {
    memmove(dst + (height - i - 1) * rowBytes, src + i * rowBytes, rowBytes);
  }
}

// workaround for semi-transparent objects making the PNG semi-transparent (even on opaque background)
void fix_alpha(unsigned char *image, size_t width, size_t height) {
  assert( image );
  size_t totalBytes = 4 * width * height;
  for (size_t i = 3 ; i < totalBytes ; i+=4) {
    if (image[i] != 0)
      image[i] = 0xFF;
  }
}

bool write_png(const char *filename, unsigned char *pixels, int width, int height)
{
  assert(filename && pixels);
  std::ofstream fstream(filename, std::ios::binary);
  if (fstream.is_open()) {
    write_png(fstream, pixels, width, height);
    fstream.close();
    return true;
  } else {
    std::cerr << "Can't open file " << filename << " for export.";
    return false;
  }
}
