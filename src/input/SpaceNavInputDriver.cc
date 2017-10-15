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

#include "input/SpaceNavInputDriver.h"
#include "input/InputDriverManager.h"

#include <spnav.h>

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

SpaceNavInputDriver::SpaceNavInputDriver()
{

}

SpaceNavInputDriver::~SpaceNavInputDriver()
{

}

void SpaceNavInputDriver::run()
{
    for (;; sleep_iter()) {
        if (spnav_open() >= 0) {
            spnav_input();
        }
    }
}

void SpaceNavInputDriver::spnav_input(void)
{
    spnav_event spn_ev;
    while (spnav_wait_event(&spn_ev)) {
        if (spn_ev.type == SPNAV_EVENT_MOTION) {
            if (spn_ev.motion.x != 0 || spn_ev.motion.y != 0 || spn_ev.motion.z != 0) {
                InputEvent *event = new InputEventTranslate(
                    0.1 * spn_ev.motion.x, 0.1 * spn_ev.motion.z, 0.1 * spn_ev.motion.y);
                InputDriverManager::instance()->postEvent(event);
            }
            if (spn_ev.motion.rx != 0 || spn_ev.motion.ry != 0 || spn_ev.motion.rz != 0) {
                InputEvent *event = new InputEventRotate(
                    0.01 * spn_ev.motion.rx, 0.01 * spn_ev.motion.rz, 0.01 * spn_ev.motion.ry);
                InputDriverManager::instance()->postEvent(event);
            }
        } else {
            if (spn_ev.button.press) {
                InputEvent *event = new InputEventButton(spn_ev.button.bnum, true);
                InputDriverManager::instance()->postEvent(event);
            } else {
                InputEvent*event = new InputEventButton(spn_ev.button.bnum, false);
                InputDriverManager::instance()->postEvent(event);
            }
        }
    }
    spnav_close();
}

void SpaceNavInputDriver::open()
{
    start();
}

void SpaceNavInputDriver::close()
{

}
