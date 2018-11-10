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

#include <iomanip>
#include <iostream>
#include <bitset>

#include "input/HidApiInputDriver.h"
#include "input/InputDriverManager.h"

static const int BUFLEN = 16;

using namespace std;

// http://www.linux-usb.org/usb.ids
// http://www.3dconnexion.eu/nc/service/faq/show_faq/7ece50ed-0b39-b57e-d3b2-4afd9420604e.html
static const struct device_id device_ids[] = {
    { 0x046d, 0xc603, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Spacemouse Plus XT"},
    { 0x046d, 0xc605, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion CADman"},
    { 0x046d, 0xc606, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Spacemouse Classic"},
    { 0x046d, 0xc621, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Spaceball 5000"},
    { 0x046d, 0xc623, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Space Traveller 3D Mouse"},
    { 0x046d, 0xc625, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Space Pilot 3D Mouse"},
    { 0x046d, 0xc626, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Space Navigator 3D Mouse"},
    { 0x046d, 0xc627, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Space Explorer 3D Mouse"},
    { 0x046d, 0xc628, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Space Navigator for Notebooks"},
    { 0x046d, 0xc629, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion SpacePilot Pro 3D Mouse"},
    { 0x046d, 0xc62b, &HidApiInputDriver::hidapi_decode_axis1, &HidApiInputDriver::hidapi_decode_button1, "3Dconnexion Space Mouse Pro"},
    { 0x256f, 0xc62e, &HidApiInputDriver::hidapi_decode_axis2, &HidApiInputDriver::hidapi_decode_button2, "3Dconnexion Space Mouse Wireless (cabled)"},
    { 0x256f, 0xc62f, &HidApiInputDriver::hidapi_decode_axis2, &HidApiInputDriver::hidapi_decode_button2, "3Dconnexion Space Mouse Wireless"},
    { 0x256f, 0xc631, &HidApiInputDriver::hidapi_decode_axis2, &HidApiInputDriver::hidapi_decode_button2, "3Dconnexion Space Mouse Pro Wireless (cabled)"},
    { 0x256f, 0xc632, &HidApiInputDriver::hidapi_decode_axis2, &HidApiInputDriver::hidapi_decode_button2, "3Dconnexion Space Mouse Pro Wireless"},
    { -1, -1, NULL, NULL, NULL},
};

HidApiInputDriver::HidApiInputDriver() : buttons(0), hid_dev(0), dev(0)
{
    name = "HidApiInputDriver";
}

HidApiInputDriver::~HidApiInputDriver()
{

}

void HidApiInputDriver::run()
{
    hidapi_input(hid_dev);
}

/*
 * Handle axis events for Space Navigator 3D Mouse.
 */
void HidApiInputDriver::hidapi_decode_axis1(const unsigned char *buf, unsigned int len)
{
    if ((buf[ 0] == 1 || buf[ 0] == 2) && len == 7) {
        // Values are in the range -10..10 at min. speed and -2595..2595 at max. speed.
        int16_t x_value = buf[ 1] | buf[ 2] << 8;
        int16_t y_value = buf[ 3] | buf[ 4] << 8;
        int16_t z_value = buf[ 5] | buf[ 6] << 8;
        if (x_value == 0 && y_value == 0 && z_value == 0) {
            return;
        }
        double x = x_value/350.0;
        double y = y_value/350.0;
        double z = z_value/350.0;
        if (buf[ 0] == 1) {
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, x));
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, y));
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, z)); 
        } else {
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3, x));
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4, y));
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5, z));
        }
    }
}

/*
 * Handle button events for Space Navigator 3D Mouse.
 */
void HidApiInputDriver::hidapi_decode_button1(const unsigned char *buf, unsigned int len)
{
    if (buf[0] == 3 && len == 3) {
        // Is either 0, 1 or 2 on MacOS.
        uint16_t bitmask = buf[1] | buf[2] << 8;
        uint16_t down = bitmask;

        std::bitset<16> bits_curr (down);
        std::bitset<16> bits_last (buttons);

        for (int i = 0;i < 16;i++) {
            if(bits_curr.test(i) != bits_last.test(i)){
                InputEvent *event = new InputEventButtonChanged(i, bits_curr.test(i));
                InputDriverManager::instance()->sendEvent(event);
            }
        }

        buttons = down;
    }
}

/*
 * Handle button events for Space Mouse Wireless. This device reports all
 * 6 axis is a single 13 byte HID message.
 * The axis value range is +/- 350 for all axis.
 */
void HidApiInputDriver::hidapi_decode_axis2(const unsigned char *buf, unsigned int len)
{
    if ((buf[0] != 1) || (len != 13)) {
        return;
    }

    for (int a = 0;a < 6;a++) {
        int16_t i = buf[2 * a + 1] | (buf[2 * a + 2] << 8);
        double val = (double)i / 350.0;
        if (fabs(val) > 0.01) {
            InputEvent *event = new InputEventAxisChanged(a, val);
            InputDriverManager::instance()->sendEvent(event);
        }
    }
}

/*
 * Handle button events for Space Mouse Wireless. This device has 2 buttons
 * reported as bit mask. On Linux the packet has a length of 3 but on Windows
 * the length is 13.
 */
void HidApiInputDriver::hidapi_decode_button2(const unsigned char *buf, unsigned int len)
{
    if ((buf[0] != 3) || (len < 3)) {
        return;
    }

    unsigned int current = buf[1];
    unsigned int changed = buttons ^ current;

    for (int idx = 0;idx < 2;idx++) {
        if (changed & (1 << idx)) {
            InputEvent *event = new InputEventButtonChanged(idx, current & (1 << idx));
            InputDriverManager::instance()->sendEvent(event);
        }
    }

    buttons = current;
}

void HidApiInputDriver::hidapi_input(hid_device* hid_dev)
{
    unsigned char buf[BUFLEN];
    unsigned int len;
    while ((len = hid_read(hid_dev, buf, BUFLEN)) > 0) {
        (this->*(dev->axis_decoder))(buf, len);
        (this->*(dev->button_decoder))(buf, len);
    }
    hid_close(hid_dev);
}

bool HidApiInputDriver::open()
{
    if (hid_init() < 0) {
        PRINTD("Can't hid_init().\n");
        return false;
    }

    for (int idx = 0;device_ids[idx].name != NULL;idx++) {
        hid_dev = hid_open(device_ids[idx].vendor_id, device_ids[idx].product_id, NULL);
        if (hid_dev != NULL) {
            dev = &device_ids[idx];
            std::stringstream sstream;
            sstream << std::setfill('0') << std::setw(4) << std::hex
                << "HidApiInputDriver ("
                << dev->vendor_id << ":" << dev->product_id
                 << " - " << dev->name
                << ")";
            name = sstream.str();
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
    return name;
}
