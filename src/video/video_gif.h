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
#pragma once

#include <gif_lib.h>

#include "video.h"

class GifVideoExport : public AbstractVideoExport
{
private:
    enum { MAX_FRAME_DELAY = 65535 };
    typedef enum { STATE_INIT, STATE_COLORMAP, STATE_OUTPUT, STATE_END } state_t;

    state_t state;
    unsigned int width, height;
    unsigned int frame_delay;

    ColorMapObject *cmap;
    QVector<QRgb> export_color_map;

    GifFileType *gif_handle;
    unsigned char *buf;

private:
    void collect_colormap(const QImage &frame);
    unsigned int find_hchange(const unsigned char *b1, const unsigned char *b2, const int start, const int end);
    unsigned int find_vchange(const unsigned char *b1, const unsigned char *b2, const unsigned int miny, const unsigned int maxy, const int start, const int end);
    bool flush_buffer(unsigned char *buf, unsigned int delay, unsigned int minx, unsigned int maxx, unsigned int miny, unsigned int maxy);

public:
    GifVideoExport(const unsigned int width = 0, unsigned const int height = 0);
    virtual ~GifVideoExport();

    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName, const double fps);
    virtual void close();
    virtual bool exportFrame(const QImage frame, const int step);
};
