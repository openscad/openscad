/*
   Translation unit holding the stb_image_write implementation.

   Vendored v1.16 from https://github.com/nothings/stb (dual MIT / public domain).
   We use it only for stbi_write_jpg_to_func(), which feeds the MJPEG frames in
   src/io/video_avi.cc -- nothing in the tree encodes JPEG otherwise, and Qt's
   encoder is unavailable to the headless CLI and the unit tests.
 */

// We only ever write through the callback form, never to a FILE*.
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
