#include <math.h>

#include "video_png.h"

PngVideoExport::PngVideoExport(const unsigned int width, const unsigned int height)
{
	this->width = width;
	this->height = height;
}

PngVideoExport::~PngVideoExport()
{
}

QString
PngVideoExport::name() const
{
	return "PNG Images";
}

AbstractVideoExport *
PngVideoExport::create(const unsigned int width, const unsigned int height) const
{
	return new PngVideoExport(width, height);
}

void
PngVideoExport::open(const QString filename)
{
	const QString f(filename.trimmed());
	
	if (f.length() == 0) {
		this->filename = "frame";
	} else {
		this->filename = filename;
	}
}

void
PngVideoExport::close()
{
}

void
PngVideoExport::exportFrame(const QImage frame, const double s, const double t)
{
	QString name;
	name.sprintf("%s%05d.png", filename.toStdString().c_str(), int(round(s*t)));
	frame.save(name, "PNG");
}
