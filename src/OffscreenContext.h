#ifndef OFFSCREENCONTEXT_H_
#define OFFSCREENCONTEXT_H_

#include <iostream>
#include <fstream>
#include <string>

struct OffscreenContext *create_offscreen_context(int w, int h);
bool teardown_offscreen_context(OffscreenContext *ctx);
bool save_framebuffer(OffscreenContext *ctx, std::ostream &output);
std::string offscreen_context_getinfo(OffscreenContext *ctx);

void bind_offscreen_context(OffscreenContext *ctx)
{
	if (ctx) fbo_bind(ctx->fbo);
}

/*
  Capture framebuffer from OpenGL and write it to the given filename as PNG.
*/
inline bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  std::ofstream fstream(filename);
  if (!fstream.is_open()) {
    std::cerr << "Can't open file " << filename << " for writing";
    return false;
  } else {
    save_framebuffer(ctx, fstream);
    fstream.close();
  }
  return true;
}

#endif
