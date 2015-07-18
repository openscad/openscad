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

static const int BUFLEN = 16;

using namespace std;

struct device_id {
    int vendor_id;
    int product_id;
    const char *name;
};

HidApiInputDriver::HidApiInputDriver() : hid_dev(0)
{

}

HidApiInputDriver::~HidApiInputDriver()
{

}

void HidApiInputDriver::run()
{
    hidapi_input(hid_dev);
}

void HidApiInputDriver::hidapi_input(hid_device* hid_dev)
{
    unsigned char buf[BUFLEN];
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

    // see http://www.linux-usb.org/usb.ids
    struct device_id device_ids[] = {
        { 0x046d, 0xc603, "3Dconnexion Spacemouse Plus XT"},
        { 0x046d, 0xc605, "3Dconnexion CADman"},
        { 0x046d, 0xc606, "3Dconnexion Spacemouse Classic"},
        { 0x046d, 0xc621, "3Dconnexion Spaceball 5000"},
        { 0x046d, 0xc623, "3Dconnexion Space Traveller 3D Mouse"},
        { 0x046d, 0xc625, "3Dconnexion Space Pilot 3D Mouse"},
        { 0x046d, 0xc626, "3Dconnexion Space Navigator 3D Mouse"},
        { 0x046d, 0xc627, "3Dconnexion Space Explorer 3D Mouse"},
        { 0x046d, 0xc628, "3Dconnexion Space Navigator for Notebooks"},
        { 0x046d, 0xc629, "3Dconnexion SpacePilot Pro 3D Mouse"},
        { 0x046d, 0xc62b, "3Dconnexion Space Mouse Pro"},
        { 0x256f, 0xc62e, "3Dconnexion Space Mouse Wireless"},
        { -1, -1, NULL},
    };

    for (int idx = 0;device_ids[idx].name != NULL;idx++) {
        hid_dev = hid_open(device_ids[idx].vendor_id, device_ids[idx].product_id, NULL);
        if (hid_dev != NULL) {
            start();
            return true;
        }
    }
    return false;
}

void HidApiInputDriver::close()
{

}

const std::string & HidApiInputDriver::get_name() const
{
    static std::string name = "HidApiInputDriver";
    return name;
}
