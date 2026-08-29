#pragma once

#include <cstdlib>
#include <iostream>

// pixels is always RGBA; with_alpha=false discards the alpha channel on output.
bool write_png(const char *filename, unsigned char *pixels, int width, int height,
               bool with_alpha = false);
bool write_png(std::ostream& output, unsigned char *pixels, int width, int height,
               bool with_alpha = false);
void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width,
                size_t height);
