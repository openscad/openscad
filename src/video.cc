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
#include "video.h"

#include "video/video_png.h"

#ifdef ENABLE_VIDEO_GIF
#include "video/video_gif.h"
#endif

#ifdef ENABLE_VIDEO_VPX
#include "video/video_vpx.h"
#endif

#ifdef ENABLE_VIDEO_XVID
#include "video/video_xvid.h"
#endif

Video::Video()
{
	exporters << boost::shared_ptr<AbstractVideoExport>(new PngVideoExport());

#ifdef ENABLE_VIDEO_GIF
	exporters << boost::shared_ptr<AbstractVideoExport>(new GifVideoExport());
#endif

#ifdef ENABLE_VIDEO_VPX
	exporters << boost::shared_ptr<AbstractVideoExport>(new VpxVideoExport());
#endif

#ifdef ENABLE_VIDEO_XVID
	exporters << boost::shared_ptr<AbstractVideoExport>(new XvidVideoExport());
#endif
}

Video::~Video()
{
}

const QStringList Video::getExporterNames()
{
	QStringList names;
	for (int a = 0;a < exporters.size();a++) {
		QString name = exporters[a].get()->name();
		names << name;
	}
	return names;
}

AbstractVideoExport * Video::getExporter(unsigned int idx, unsigned int width, unsigned int height)
{
	AbstractVideoExport * exporterTemplate = exporters[idx].get();
	return exporterTemplate->create(width, height);
}
