#include <math.h>

#include "video_png.h"

PngVideo::PngVideo(const int width, const int height)
{
	this->width = width;
	this->height = height;
}

PngVideo::~PngVideo()
{
}

void
PngVideo::open(const QString fileName)
{
	
}

void
PngVideo::close()
{
	
}

void
PngVideo::exportFrame(const QImage frame, const double s, const double t)
{
	QString filename;
	filename.sprintf("frame%05d.png", int(round(s*t)));
	frame.save(filename, "PNG");
}
