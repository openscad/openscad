#ifndef OFFSCREENCONTEXT_H_
#define OFFSCREENCONTEXT_H_

#include <iostream>         // for error output
#include <string>

struct OffscreenContext *create_offscreen_context(int w, int h);
void bind_offscreen_context(OffscreenContext *ctx);
bool teardown_offscreen_context(OffscreenContext *ctx);
bool save_framebuffer(OffscreenContext *ctx, const char *filename);
std::string offscreen_context_getinfo(OffscreenContext *ctx);

#endif
