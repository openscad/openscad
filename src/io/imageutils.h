#pragma once

#include <cstdlib>
#include <iostream>

bool write_png(const char *filename, unsigned char *pixels, int width, int height);
bool write_png(std::ostream& output, unsigned char *pixels, int width, int height);
/*!
   Write a 16-bit greyscale PNG. Samples are big-endian, as PNG stores them.

   Always lodepng, on every platform: the CoreGraphics writer used for color
   output on macOS is 8 bits per component, and lodepng is compiled
   unconditionally, so there is no reason to fork this per platform.
 */
bool write_png_gray16(std::ostream& output, const unsigned char *pixels, int width, int height);
void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width,
                size_t height);
