#include <math.h>

#include "video_gif.h"

GifVideo::GifVideo(const int width, const int height)
{
	this->width = width & ~1;
	this->height = height & ~1;
	this->frame_delay = 0;
	this->cmap = NULL;
	this->buf = NULL;
	this->state = STATE_INIT;
}

GifVideo::~GifVideo()
{
}

void
GifVideo::open(const QString fileName)
{
	gif_handle = EGifOpenFileName(fileName.toStdString().c_str(), false, NULL);
	EGifSetGifVersion(gif_handle, true);
}

void
GifVideo::close()
{
	if (gif_handle != NULL) {
		if (buf) {
			flush_buffer(buf, frame_delay);
		}
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

void
GifVideo::exportFrame(const QImage frame, const double s, const double t)
{
	int frameNr = int(round(s * t));

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
			return;
		}
		break;
	case STATE_END:
		return;
	}

	switch (state) {
	case STATE_COLORMAP: {
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

			if (EGifPutScreenDesc(gif_handle, width, height, 256, 0, cmap) != GIF_OK)
				return;

			if (EGifPutExtensionLeader(gif_handle, 0xFF) != GIF_OK)
				return;

			if (EGifPutExtensionBlock(gif_handle, 11, EXT_NETSCAPE) != GIF_OK)
				return;

			if (EGifPutExtensionBlock(gif_handle, 3, EXT_NETSCAPE_LOOP) != GIF_OK)
				return;

			if (EGifPutExtensionTrailer(gif_handle) != GIF_OK)
				return;
		}

		const QImage scaled = frame.scaled(width, height).convertToFormat(QImage::Format_Indexed8, export_color_map);

		int _fps_den = 1;
		frame_delay += 2 * _fps_den;
		if (frame_delay > MAX_FRAME_DELAY)
			frame_delay = MAX_FRAME_DELAY;

		if (buf == NULL) {
			buf = new unsigned char[width * height];
			for (unsigned int y = 0; y < height; y++) {
				memcpy(buf + y * width, scaled.scanLine(y), width);
			}
			return;
		}

		if (buf) {
			flush_buffer(buf, frame_delay);
			for (unsigned int y = 0; y < height; y++) {
				memcpy(buf + y * width, scaled.scanLine(y), width);
			}
		} else {
			flush_buffer((unsigned char *) scaled.bits(), frame_delay);
		}

		frame_delay = 0;
	}
		break;
	default:
		break;
	}
}

bool
GifVideo::flush_buffer(unsigned char *buf, unsigned int delay)
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
	 *         hundredths (1/100) of a second to wait before continuing with the
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

	if (EGifPutImageDesc(gif_handle, 0, 0, width, height, 0, cmap) != GIF_OK)
		return false;

	if (EGifPutLine(gif_handle, buf, width * height) != GIF_OK)
		return false;
	
	return true;
}

