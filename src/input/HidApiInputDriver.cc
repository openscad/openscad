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

#include <hidapi.h>

#include "input/HidApiInputDriver.h"
#include "input/InputDriverManager.h"

#define	WSTRLEN		256
#define	BUFLEN		16
#define	VENDOR_ID	0x046d
#define	PRODUCT_ID	0xc626

using namespace std;

static void
sleep_iter(void)
{
    struct timespec schnarch = {1, 0};
    while (nanosleep(&schnarch, &schnarch) < 0) {
    };
}

HidApiInputDriver::HidApiInputDriver() : hid_dev(0)
{

}

HidApiInputDriver::~HidApiInputDriver()
{

}

void HidApiInputDriver::run()
{
    for (;;sleep_iter()) {
        hidapi_input(hid_dev);
    }
}

void HidApiInputDriver::hidapi_input(hid_device* hid_dev)
{
    unsigned char buf[ BUFLEN];
    int len;
    uint16_t buttons = 0;
    while ((len = hid_read(hid_dev, buf, BUFLEN)) > 0) {
        InputEvent *event = 0;
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
            InputDriverManager::instance()->postEvent(event);
        }
    }
    hid_close(hid_dev);
}

bool HidApiInputDriver::open()
{
    if (hid_init() < 0) {
        PRINTD("Can't hid_init().\n");
        return false;
    }
    // Try a list of product IDs. But up to now I know of only one.
    hid_dev = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    if (hid_dev == NULL) {
        return false;
    }
    start();
    return true;
}

void HidApiInputDriver::close()
{

}

const std::string & HidApiInputDriver::get_name() const
{
    static std::string name = "HidApiInputDriver";
    return name;
}
