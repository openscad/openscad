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

#include <xvid.h>

#include "video.h"

class XvidVideoExport : public AbstractVideoExport
{
private:
    bool init_ok;
    unsigned int width, height;

    FILE *f;
    unsigned char *buf;
    xvid_gbl_init_t      _gbl_init;
    xvid_enc_create_t    _enc_create;
    xvid_plugin_single_t _plugin_single;
    xvid_enc_plugin_t    _plugins[1];
    
public:
    XvidVideoExport(const unsigned int width = 0, const unsigned int height = 0);
    virtual ~XvidVideoExport();
    
    virtual QString name() const;
    virtual AbstractVideoExport * create(const unsigned int width, const unsigned int height) const;

    virtual void open(const QString fileName);
    virtual void close();
    virtual void exportFrame(const QImage frame, const double s, const double t);
};
