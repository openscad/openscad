#include "io/imageutils.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <fstream>
#include "utils/printutils.h"

void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width,
                size_t height)
{
  assert(src && dst);
  auto rowBytes = pixelsize * width;
  for (auto i = 0ul; i < height; ++i) {
    memmove(dst + (height - i - 1) * rowBytes, src + i * rowBytes, rowBytes);
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
    LOG(_("Can't open file \"%1$s\" for PNG export: %2$s [%3$i], working directory is %4$s"), filename,
        strerror(errno), errno, fs::current_path());
    return false;
  }
}
