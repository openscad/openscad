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
#include <stdio.h>

#include "video_vpx.h"

#define vpx_interface (vpx_codec_vp8_cx())

#define fourcc    0x30385056

#define IVF_FILE_HDR_SZ  (32)
#define IVF_FRAME_HDR_SZ (12)

static int rgb32_to_i420(int width, int height, const char * src, char * dst)
{
	unsigned char * dst_y_even;
	unsigned char * dst_y_odd;
	unsigned char * dst_u;
	unsigned char * dst_v;
	const unsigned char *src_even;
	const unsigned char *src_odd;
	int i, j;

	src_even = (const unsigned char *)src;
	src_odd = src_even + width * 4;

	dst_y_even = (unsigned char *)dst;
	dst_y_odd = dst_y_even + width;
	dst_u = dst_y_even + width * height;
	dst_v = dst_u + ((width * height) >> 2);

	for ( i = 0; i < height / 2; ++i )
	{
		for ( j = 0; j < width / 2; ++j )
		{
			short r, g, b;
			b = *src_even++;
			g = *src_even++;
			r = *src_even++;
			++src_even;
			*dst_y_even++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;

			*dst_u++ = (( r * -38 - g * 74 + b * 112 + 128 ) >> 8 ) + 128;
			*dst_v++ = (( r * 112 - g * 94 - b * 18 + 128 ) >> 8 ) + 128;

			b = *src_even++;
			g = *src_even++;
			r = *src_even++;
			++src_even;
			*dst_y_even++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;

			b = *src_odd++;
			g = *src_odd++;
			r = *src_odd++;
			++src_odd;
			*dst_y_odd++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;

			b = *src_odd++;
			g = *src_odd++;
			r = *src_odd++;
			++src_odd;
			*dst_y_odd++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;
		}

		dst_y_even += width;
		dst_y_odd += width;
		src_even += width * 4;
		src_odd += width * 4;
	}

	return 0;
}

VpxVideoExport::VpxVideoExport(const unsigned int width, const unsigned int height)
{
	this->width = width & ~1;
	this->height = height & ~1;
}

VpxVideoExport::~VpxVideoExport()
{
}

QString
VpxVideoExport::name() const
{
	return "WebM video (VP8)";
}

AbstractVideoExport *
VpxVideoExport::create(const unsigned int width, const unsigned int height) const
{
	return new VpxVideoExport(width, height);
}

void
VpxVideoExport::open(const QString fileName, const double fps)
{
	QString name = QString("%1.webm").arg(fileName);

	init_ok = false;
	if (!vpx_img_alloc(&raw, VPX_IMG_FMT_I420, width, height, 1)) {
		return;
	}

	res = vpx_codec_enc_config_default(vpx_interface, &cfg, 0);
	if (res) {
		printf("Failed to get config: %s\n", vpx_codec_err_to_string(res));
		return;
	}

	/* Update the default configuration with our settings */
	//cfg.rc_target_bitrate = (width * height * cfg.rc_target_bitrate) / cfg.g_w / cfg.g_h / 4;
	cfg.rc_target_bitrate = (1600L * width * height) / (1920L * 1080L);
	cfg.g_w = width;
	cfg.g_h = height;
	cfg.g_pass = VPX_RC_ONE_PASS;

	//write_ivf_file_header(outfile, &cfg, 0);
	memset(&ebml, 0, sizeof(EbmlGlobal));
	ebml.last_pts_ms = -1;
	ebml.stream = fopen(name.toStdString().c_str(), "wb");
	if (ebml.stream == NULL)
	{
		return;
	}
	vpx_rational arg_framerate = {30, 1};
	Ebml_WriteWebMFileHeader(&ebml, &cfg, &arg_framerate);

	/* Initialize codec */
	if (vpx_codec_enc_init(&codec, vpx_interface, &cfg, 0))
		return;

	frame_cnt = 0;

	printf("Using %s with bitrate %u @ %dx%d\n", vpx_codec_iface_name(vpx_interface), cfg.rc_target_bitrate, width, height);
}

void
VpxVideoExport::close()
{
	Ebml_WriteWebMFileFooter(&ebml, 0);
	fclose(ebml.stream);

	vpx_img_free(&raw);
	if (vpx_codec_destroy(&codec))
		return;
}

bool
VpxVideoExport::exportFrame(const QImage frame, const int)
{
	const QImage scaled = frame.scaled(width, height);
	rgb32_to_i420(width, height, (const char *)scaled.bits(), (char *)raw.planes[0]);

	vpx_codec_err_t r = vpx_codec_encode(&codec, &raw, frame_cnt, 1, 0, VPX_DL_GOOD_QUALITY);
	if (r) {
		printf("can't encode\n");
		return false;
	}

	vpx_codec_iter_t iter = NULL;
	const vpx_codec_cx_pkt_t *pkt;
	while ((pkt = vpx_codec_get_cx_data(&codec, &iter))) {
		Ebml_WriteWebMBlock(&ebml, &cfg, pkt);
	}
	frame_cnt++;
	return true;
}
