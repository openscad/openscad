#ifndef OFFSCREENCONTEXT_H_
#define OFFSCREENCONTEXT_H_

#include <iostream>
#include <fstream>
#include <string>
#include "fbo.h"

struct OffscreenContext *create_offscreen_context(int w, int h);
bool teardown_offscreen_context(OffscreenContext *ctx);
bool save_framebuffer(OffscreenContext *ctx, const char * filename);
bool save_framebuffer(OffscreenContext *ctx, std::ostream &output);
std::string offscreen_context_getinfo(OffscreenContext *ctx);

#endif
