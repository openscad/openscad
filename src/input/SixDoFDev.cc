/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
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

/*
 *  Initial implementation by Jochen Kunz and Gert Menke provided as
 *  Public Domain.
 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <iostream>

#include <QWidget>
#include <QApplication>

#include <hidapi.h>
#include <spnav.h>

#include "printutils.h"
#include "input/SixDoFDev.h"


#define	WSTRLEN		256
#define	BUFLEN		16
#define	VENDOR_ID	0x046d
#define	PRODUCT_ID	0xc626

using namespace std;

static void
sleep_iter( void) {
	struct timespec schnarch = { 1, 0};
	while (nanosleep( &schnarch, &schnarch) < 0) {};
}

SixDoFDev::SixDoFDev() {

}

SixDoFDev::~SixDoFDev() {

}

void
SixDoFDev::run() {
	if (hid_init() < 0) {
		PRINTD( "Can't hid_init().\n");
		return;
	}
	for (;; sleep_iter()) {
		if (spnav_open() >= 0) {
			spnav_input();
		}
		// Try a list of product IDs. But up to now I know of only one.
		hid_device* hid_dev;
		hid_dev = hid_open( VENDOR_ID, PRODUCT_ID, NULL);
		if (hid_dev != NULL) {
			hidapi_input( hid_dev);
		}
	}
}

void
SixDoFDev::spnav_input( void) {
	spnav_event spn_ev;
	while (spnav_wait_event( &spn_ev)) {
		QWidget* widget = QApplication::activeWindow();
		if (widget == 0) {
			continue;
		}
		if (spn_ev.type == SPNAV_EVENT_MOTION) {
			if (spn_ev.motion.x != 0 || spn_ev.motion.y != 0 || spn_ev.motion.z != 0) {
				QEvent* event = new InputEventTranslate(
					0.1 * spn_ev.motion.x, 0.1 * spn_ev.motion.z, 0.1 * spn_ev.motion.y);
				QCoreApplication::postEvent( widget, event);
			}
			if (spn_ev.motion.rx != 0 || spn_ev.motion.ry != 0 || spn_ev.motion.rz != 0) {
				QEvent* event = new InputEventRotate(
					0.01 * spn_ev.motion.rx, 0.01 * spn_ev.motion.rz, 0.01 * spn_ev.motion.ry);
				QCoreApplication::postEvent( widget, event);
			}
		} else {
			if (spn_ev.button.press) {
				QEvent* event = new InputEventButton(spn_ev.button.bnum, true);
				QCoreApplication::postEvent( widget, event);
			} else {
				QEvent* event = new InputEventButton(spn_ev.button.bnum, false);
				QCoreApplication::postEvent( widget, event);
			}
		}
	}
	spnav_close();
}

void
SixDoFDev::hidapi_input( hid_device* hid_dev) {
	unsigned char buf[ BUFLEN];
	int len;
	uint16_t buttons = 0;
	while ((len = hid_read( hid_dev, buf, BUFLEN)) > 0) {
		// QWidget* widget = QApplication::focusWidget(); doesn't do the tric.
		QWidget* widget = QApplication::activeWindow();
		if (widget == 0) {
			continue;
		}
		QEvent* event = 0;
		if ((buf[ 0] == 1 || buf[ 0] == 2) && len == 7) {
			// Values are in the range -10..10 at min. speed and -2595..2595 at max. speed.
			int16_t x_value = buf[ 1] | buf[ 2] << 8;
			int16_t y_value = buf[ 3] | buf[ 4] << 8;
			int16_t z_value = buf[ 5] | buf[ 6] << 8;
			if (x_value == 0 && y_value == 0 && z_value == 0) {
				continue;
			}
			if (buf[ 0] == 1) {
				event = new InputEventTranslate(0.1 * x_value, 0.1 * y_value, 0.1 * z_value);
			} else {
				event = new InputEventRotate(0.01 * x_value, 0.01 * y_value, 0.01 * z_value);
			}
		}
		if (buf[ 0] == 3 && len == 3) {
			// Is either 0, 1 or 2 on MacOS.
			uint16_t bitmask = buf[ 1] | buf[ 2] << 8;
			uint16_t down = bitmask;
			uint16_t up = 0;
			if (bitmask == 0) {
				up = buttons;
				buttons = 0;
			} else {
				buttons |= bitmask;
			}
			if (down != 0 || up != 0) {
				event = new InputEventButton(down, up);
			}
		}
		if (event != 0) {
			QCoreApplication::postEvent( widget, event);
		}
	}
	hid_close( hid_dev);
}

void SixDoFDev::open()
{
    start();
}

void SixDoFDev::close()
{

}

SixDoFDev spacenav;
