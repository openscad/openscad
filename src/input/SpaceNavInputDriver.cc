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
#include <unistd.h>

SpaceNavInputDriver::SpaceNavInputDriver()
{

}

SpaceNavInputDriver::~SpaceNavInputDriver()
{

}

void SpaceNavInputDriver::run()
{
    while (spnav_input()) {
        QThread::msleep(20);
        spnav_remove_events(SPNAV_EVENT_MOTION);
    }
}

/*
 * Handle events from the spacenavd daemon. The method blocks until at least
 * one event is available and then processes all events until the queue is
 * empty.
 */
bool SpaceNavInputDriver::spnav_input(void)
{
    spnav_event ev;

    if (spnav_wait_event(&ev) == 0) {
        return false;
    }

    do {
        if (ev.type == SPNAV_EVENT_MOTION) {
            if (true) {
                // dominant axis only
                int m=ev.motion.x;
                if (abs(m) < abs(ev.motion.y)) m=ev.motion.y;
                if (abs(m) < abs(ev.motion.z)) m=ev.motion.z;
                if (abs(m) < abs(ev.motion.rx)) m=ev.motion.rx;
                if (abs(m) < abs(ev.motion.ry)) m=ev.motion.ry;
                if (abs(m) < abs(ev.motion.rz)) m=ev.motion.rz;

                if (ev.motion.x == m) {                ev.motion.y=0; ev.motion.z=0; ev.motion.rx=0; ev.motion.ry=0; ev.motion.rz=0; }
                if (ev.motion.y == m) { ev.motion.x=0;                ev.motion.z=0; ev.motion.rx=0; ev.motion.ry=0; ev.motion.rz=0; }
                if (ev.motion.z == m) { ev.motion.x=0; ev.motion.y=0;                ev.motion.rx=0; ev.motion.ry=0; ev.motion.rz=0; }
                if (ev.motion.rx== m) { ev.motion.x=0; ev.motion.y=0; ev.motion.z=0;                 ev.motion.ry=0; ev.motion.rz=0; }
                if (ev.motion.ry== m) { ev.motion.x=0; ev.motion.y=0; ev.motion.z=0; ev.motion.rx=0;                 ev.motion.rz=0; }
                if (ev.motion.rz== m) { ev.motion.x=0; ev.motion.y=0; ev.motion.z=0; ev.motion.rx=0; ev.motion.ry=0;                 }
            }

            if (ev.motion.x != 0 || ev.motion.y != 0 || ev.motion.z != 0) {
                InputEvent *event = new InputEventTranslate(0.1 * ev.motion.x, 0.1 * ev.motion.z, 0.1 * ev.motion.y);
                InputDriverManager::instance()->sendEvent(event);
            }
            if (ev.motion.rx != 0 || ev.motion.ry != 0 || ev.motion.rz != 0) {
                InputEvent *event = new InputEventRotate(0.01 * ev.motion.rx, 0.01 * ev.motion.rz, 0.01 * ev.motion.ry);
                InputDriverManager::instance()->sendEvent(event);
            }
        } else if (ev.type == SPNAV_EVENT_BUTTON) {
            InputEvent *event = new InputEventButtonChanged(ev.button.bnum, ev.button.press);
            InputDriverManager::instance()->sendEvent(event);
        }
    } while (spnav_poll_event(&ev));
    return true;
}

bool SpaceNavInputDriver::open()
{
    if (spnav_open() < 0) {
        return false;
    }
    start();
    return true;
}

void SpaceNavInputDriver::close()
{

}

const std::string & SpaceNavInputDriver::get_name() const
{
    static std::string name = "SpaceNavInputDriver";
    return name;
}