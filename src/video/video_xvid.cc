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

#include "video_xvid.h"

XvidVideoExport::XvidVideoExport(const unsigned int width, const unsigned int height)
{
	this->width = width & ~1;
	this->height = height & ~1;

	memset(&_gbl_init, 0, sizeof (xvid_gbl_init_t));
	_gbl_init.version = XVID_VERSION;
	xvid_global(NULL, XVID_GBL_INIT, &_gbl_init, NULL);
}

XvidVideoExport::~XvidVideoExport()
{
}

QString
XvidVideoExport::name() const
{
	return "MP4";
}

AbstractVideoExport *
XvidVideoExport::create(const unsigned int width, const unsigned int height) const
{
	return new XvidVideoExport(width, height);
}

void
XvidVideoExport::open(const QString fileName, const double fps)
{
	QString name = QString("%1.mp4").arg(fileName);

	init_ok = false;

	double fps_den = 1;
	memset(&_enc_create, 0, sizeof (xvid_enc_create_t));
	_enc_create.version = XVID_VERSION;
	_enc_create.width = width;
	_enc_create.height = height;
	_enc_create.zones = NULL;
	_enc_create.fincr = fps_den;
	_enc_create.fbase = 50;
	_enc_create.max_key_interval = 500 / fps_den;
	_enc_create.bquant_ratio = 150;
	_enc_create.bquant_offset = 100;

	memset(&_plugin_single, 0, sizeof(xvid_plugin_single_t));
	_plugin_single.version = XVID_VERSION;

	_plugins[0].func  = xvid_plugin_single;
	_plugins[0].param = &_plugin_single;

	 _enc_create.plugins = _plugins;
	 _enc_create.num_plugins = 1;

	 xvid_encore(NULL, XVID_ENC_CREATE, &_enc_create, NULL);

	f = fopen(name.toStdString().c_str(), "wb+");
	if (f == NULL)
	  return;
	
	buf = new unsigned char[width * height * 4];

	init_ok = true;
}

void
XvidVideoExport::close()
{
	xvid_encore(_enc_create.handle, XVID_ENC_DESTROY, NULL, NULL);
}

bool
XvidVideoExport::exportFrame(const QImage frame, const int)
{
	const QImage scaled = frame.scaled(width, height).convertToFormat(QImage::Format_RGB32);

	xvid_enc_frame_t enc_frame;
	memset(&enc_frame, 0, sizeof(xvid_enc_frame_t));

	const uchar *bits = scaled.bits();
	unsigned char *bitstream = new unsigned char[width * height * 4];
	enc_frame.version = XVID_VERSION;
	enc_frame.bitstream = bitstream;
	enc_frame.length = -1;
	enc_frame.input.plane[0] = (void *)bits;
	enc_frame.input.csp = XVID_CSP_BGRA;
	enc_frame.input.stride[0] = scaled.bytesPerLine();
	enc_frame.vol_flags = 0;
	enc_frame.vop_flags = XVID_VOP_HALFPEL | XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED;
	enc_frame.type = XVID_TYPE_AUTO;
	enc_frame.quant = 0.8;
	enc_frame.motion = XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 | XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8;

	int size = xvid_encore(_enc_create.handle, XVID_ENC_ENCODE, &enc_frame, NULL);
	bool write_ok = fwrite(bitstream, 1, size, f) == (size_t)size;
	delete bitstream;
	return write_ok;
}
