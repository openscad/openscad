/*

   Create an NULL OpenGL context that doesnt actually use any OpenGL code,
   and can be compiled on a system without OpenGL.

 */

#include <vector>

#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"

#include <map>
#include <string>
#include <sstream>

using namespace std;

struct OffscreenContext
{
	int width;
	int height;
};

void offscreen_context_init(OffscreenContext &ctx, int width, int height)
{
	ctx.width = width;
	ctx.height = height;
}

string offscreen_context_getinfo(OffscreenContext *ctx)
{
	return string("NULLGL");
}

OffscreenContext *create_offscreen_context(int w, int h)
{
	OffscreenContext *ctx = new OffscreenContext;
	offscreen_context_init(*ctx, w, h);
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
	return true;
}

bool save_framebuffer(OffscreenContext *ctx, char const *filename)
{
	std::ofstream fstream(filename, std::ios::out | std::ios::binary);
	if (!fstream.is_open()) {
		std::cerr << "Can't open file " << filename << " for writing";
		return false;
	}
	else {
		save_framebuffer(ctx, fstream);
		fstream.close();
	}
	return true;
}

bool save_framebuffer(OffscreenContext *ctx, std::ostream &output)
{
	output << "NULLGL framebuffer";
	return true;
}

