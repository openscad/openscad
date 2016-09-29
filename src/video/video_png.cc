/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2016 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
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
PngVideoExport::open(const QString filename, const double fps)
{
	const QString f(filename.trimmed());
	
	if (f.length() == 0) {
		this->filename = "frame";
	} else {
		this->filename = filename;
	}

	this->start_frame = -1;
	this->first_frame = true;
}

void
PngVideoExport::close()
{
}

bool
PngVideoExport::exportFrame(const QImage frame, const int step)
{
	if (this->first_frame) {
		this->start_frame = step;
		this->first_frame = false;
	} else {
		if (this->start_frame == step) {
			return false;
		}
	}

	QString name;
	name.sprintf("%s%05d.png", filename.toStdString().c_str(), step);
	frame.save(name, "PNG");
	return true;
}
