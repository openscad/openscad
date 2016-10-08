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

#include "video_gif.h"
#include "printutils.h"

GifVideoExport::GifVideoExport(const unsigned int width, unsigned const int height)
{
	this->width = width & ~1;
	this->height = height & ~1;
	this->frame_delay = 0;
	this->cmap = NULL;
	this->buf = NULL;
	this->state = STATE_INIT;
}

GifVideoExport::~GifVideoExport()
{
}

QString
GifVideoExport::name() const
{
	return "Animated GIF";
}

AbstractVideoExport *
GifVideoExport::create(const unsigned int width, const unsigned int height) const
{
	return new GifVideoExport(width, height);
}

void
GifVideoExport::open(const QString fileName, const double fps)
{
	this->fps = fps;
	const QString name = QString("%1.gif").arg(fileName);
	gif_handle = EGifOpenFileName(name.toStdString().c_str(), false, NULL);

	PRINTDB("GifVideoExport::open(): '%s'", name.toStdString().c_str());
}

void
GifVideoExport::close()
{
	PRINTD("GifVideoExport::close()");
	if (gif_handle != NULL) {
		EGifCloseFile(gif_handle, NULL);
		gif_handle = NULL;
	}
	if (cmap != NULL) {
		GifFreeMapObject(cmap);
		cmap = NULL;
	}
	if (buf != NULL) {
		delete[] buf;
		buf = NULL;
	}
}

bool
GifVideoExport::exportFrame(const QImage frame, const int frameNr)
{
	switch (state) {
	case STATE_INIT:
		if (frameNr == 0) {
			state = STATE_COLORMAP;
		}
		break;
	case STATE_COLORMAP:
		if (frameNr == 0) {
			state = STATE_OUTPUT;
		}
		break;
	case STATE_OUTPUT:
		if (frameNr == 0) {
			state = STATE_END;
			close();
			return false;
		}
		break;
	case STATE_END:
		return false;
	}

	switch (state) {
	case STATE_COLORMAP:
		collect_colormap(frame);
		break;
	case STATE_OUTPUT: {
		/*
		 * Netscape Application Extension
		 * (see: http://www.let.rug.nl/~kleiweg/gif/netscape.html)
		 *
		 * byte   1       : 33 (hex 0x21) GIF Extension code
		 * byte   2       : 255 (hex 0xFF) Application Extension Label
		 * byte   3       : 11 (hex 0x0B) Length of Application Block
		 *                  (eleven bytes of data to follow)
		 * bytes  4 to 11 : "NETSCAPE"
		 * bytes 12 to 14 : "2.0"
		 * byte  15       : 3 (hex 0x03) Length of Data Sub-Block
		 *                  (three bytes of data to follow)
		 * byte  16       : 1 (hex 0x01)
		 * bytes 17 to 18 : 0 to 65535, an unsigned integer in
		 *                  lo-hi byte format. This indicate the
		 *                  number of iterations the loop should
		 *                  be executed.
		 * byte  19       : 0 (hex 0x00) a Data Sub-Block Terminator.
		 */
		static unsigned char EXT_NETSCAPE[] = {'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0'};
		static unsigned char EXT_NETSCAPE_LOOP[] = {1, 0, 0}; // loop counter = 0

		if (frameNr == 0) {
			export_color_map.clear();
			for (int a = 0; a < 256; a++) {
				const QRgb rgb = qRgb(cmap->Colors[a].Red, cmap->Colors[a].Green, cmap->Colors[a].Blue);
				export_color_map.append(rgb);
			}

			EGifSetGifVersion(gif_handle, true);

			if (EGifPutScreenDesc(gif_handle, width, height, 256, 0, cmap) != GIF_OK)
				return false;

			if (EGifPutExtensionLeader(gif_handle, 0xFF) != GIF_OK)
				return false;

			if (EGifPutExtensionBlock(gif_handle, 11, EXT_NETSCAPE) != GIF_OK)
				return false;

			if (EGifPutExtensionBlock(gif_handle, 3, EXT_NETSCAPE_LOOP) != GIF_OK)
				return false;

			if (EGifPutExtensionTrailer(gif_handle) != GIF_OK)
				return false;
		}

		const QImage scaled = frame.scaled(width, height).convertToFormat(QImage::Format_Indexed8, export_color_map);
		unsigned char *cur = new unsigned char[width * height];
		for (unsigned int y = 0;y < height;y++) {
			memcpy(cur + y * width, scaled.scanLine(y), width);
		}

		frame_delay += 100.0 / fps;
		unsigned int cur_frame_delay = frame_delay;

		bool is_first_frame = buf == NULL;
		bool reached_max_frame_delay = cur_frame_delay > MAX_FRAME_DELAY;

		bool set_buffer = false;
		if (is_first_frame || reached_max_frame_delay) {
			flush_buffer(cur, cur_frame_delay, 0, width, 0, height);
			set_buffer = true;
		} else {
			unsigned int miny = find_hchange(cur, buf, 0, height - 1);
			unsigned int maxy = find_hchange(cur, buf, height - 1, 0);

			if (miny <= maxy) {
				unsigned int minx = find_vchange(cur, buf, miny, maxy, 0, width - 1);
				unsigned int maxx = find_vchange(cur, buf, miny, maxy, width - 1, 0);
				if (minx <= maxx) {
					flush_buffer(cur, cur_frame_delay, minx, maxx + 1, miny, maxy + 1);
					set_buffer = true;
				}
			}
		}

		if (set_buffer) {
			delete buf;
			buf = cur;
			frame_delay = 0;
		}
	}
		break;
	default:
		break;
	}

	return true;
}

unsigned int
GifVideoExport::find_hchange(const unsigned char *b1, const unsigned char *b2, const unsigned int start, const unsigned int end)
{
	int step = start < end ? 1 : -1;

	unsigned int result = start;
	for (unsigned int y = start;y != end;y += step) {
		if ((memcmp(b1 + y * width, b2 + y * width, width)) != 0) {
			break;
		}
		result = y;
	}
	return result;
}

unsigned int
GifVideoExport::find_vchange(const unsigned char *b1, const unsigned char *b2, const unsigned int miny, const unsigned int maxy, const unsigned int start, const unsigned int end)
{
	int step = start < end ? 1 : -1;

	unsigned int result = start;
	for (unsigned int x = start;x != end;x += step) {
		bool changed = false;
		for (unsigned int y = miny;y <= maxy;y++) {
			if (*(b1 + y * width + x) != *(b2 + y * width + x)) {
				changed = true;
				break;
			}
		}
		if (changed) {
			break;
		}
		result = x;
	}
	return result;
}

void
GifVideoExport::collect_colormap(const QImage &frame)
{
	QVector<QRgb> frameColorMap = frame.convertToFormat(QImage::Format_Indexed8).colorTable();

	ColorMapObject *colormap = GifMakeMapObject(256, NULL);
	for (int a = 0; a < 256; a++) {
		if (a < frameColorMap.size()) {
			const QRgb rgb = frameColorMap.at(a);
			colormap->Colors[a].Red = qRed(rgb);
			colormap->Colors[a].Green = qGreen(rgb);
			colormap->Colors[a].Blue = qBlue(rgb);
		} else {
			colormap->Colors[a].Red = 0;
			colormap->Colors[a].Green = 0;
			colormap->Colors[a].Blue = 0;
		}
	}

	if (cmap == NULL) {
		cmap = colormap;
	} else {
		GifPixelType mapping[256];
		ColorMapObject *unionmap = GifUnionColorMap(cmap, colormap, mapping);
		if (unionmap != NULL) {
			GifFreeMapObject(cmap);
			cmap = unionmap;
		}
		GifFreeMapObject(colormap);
	}
}

bool
GifVideoExport::flush_buffer(unsigned char *buf, unsigned int delay, unsigned int minx, unsigned int maxx, unsigned int miny, unsigned int maxy)
{
	/*
	 * Graphic Control Extension
	 * (see: http://local.wasp.uwa.edu.au/~pbourke/dataformats/gif/)
	 *
	 * byte 1: | 3 bit Reserved | 3 bit Disposal Method | User Input | Transparent |
	 *         Disposal Method:
	 *         0 -   No disposal specified. The decoder is
	 *               not required to take any action.
	 *         1 -   Do not dispose. The graphic is to be left
	 *               in place.
	 *         2 -   Restore to background color. The area used by the
	 *               graphic must be restored to the background color.
	 *         3 -   Restore to previous. The decoder is required to
	 *               restore the area overwritten by the graphic with
	 *               what was there prior to rendering the graphic.
	 *         4-7 -    To be defined.
	 *         User Input:
	 *         0 -   User input is not expected.
	 *         1 -   User input is expected.
	 *         Transparent:
	 *         0 -   Transparent Index is not given.
	 *         1 -   Transparent Index is given.
	 * byte 2 & 3:
	 *         Delay Time - If not 0, this field specifies the number of
	 *         hundredth (1/100) of a second to wait before continuing with the
	 *         processing of the Data Stream. The clock starts ticking immediately
	 *         after the graphic is rendered. This field may be used in
	 *         conjunction with the User Input Flag field.
	 * byte 4:
	 *         Transparency Index - The Transparency Index is such that when
	 *         encountered, the corresponding pixel of the display device is not
	 *         modified and processing goes on to the next pixel. The index is
	 *         present if and only if the Transparency Flag is set to 1.
	 */
	unsigned char EXT_GCE[] = {0, (unsigned char)(delay & 0xff), (unsigned char)((delay >> 8) & 0xff), 0};

	if (EGifPutExtension(gif_handle, 0xF9, 4, EXT_GCE) != GIF_OK)
		return false;

	if (EGifPutImageDesc(gif_handle, minx, miny, maxx - minx, maxy - miny, 0, cmap) != GIF_OK)
		return false;

	PRINTDB("GifVideoExport::flush_buffer(): x = %d-%d (%d), y = %d-%d (%d), delay = %d",
		minx % maxx % (maxx - minx) %
		miny % maxy % (maxy - miny) %
		delay);

	for (unsigned int y = miny;y < maxy;y++) {
		if (EGifPutLine(gif_handle, buf + y * width + minx, maxx - minx) != GIF_OK)
			return false;
	}
	
	return true;
}

