#pragma once

#include <cstdlib>
#include <iostream>

bool write_png(const char *filename, unsigned char *pixels, int width, int height);
bool write_png(std::ostream& output, unsigned char *pixels, int width, int height);
void flip_image(const unsigned char *src, unsigned char *dst, size_t pixelsize, size_t width, size_t height);
