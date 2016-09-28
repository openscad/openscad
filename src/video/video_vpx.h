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

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

#include "video.h"
#include "EbmlWriter.h"

class VpxVideoExport : public AbstractVideoExport
{
private:
    bool init_ok;
    unsigned int width, height;
    int frame_cnt;
    EbmlGlobal ebml;
    vpx_codec_ctx_t      codec;
    vpx_codec_enc_cfg_t  cfg;
    vpx_image_t          raw;
    vpx_codec_err_t      res;

public:
    VpxVideoExport(const unsigned int width = 0, const unsigned int height = 0);
    virtual ~VpxVideoExport();

    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
