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

#include "gui/input/SpaceNavInputDriver.h"
#include "gui/input/InputDriverManager.h"
#include "utils/printutils.h"

#include <QThread>
#include <spnav.h>
#include <unistd.h>
#include <string>

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
bool SpaceNavInputDriver::spnav_input()
{
  spnav_event ev;

  // The low level driver seems to inhibit events in the dead zone, so if we
  // enter that case, make sure to zero out our axis values.
  bool have_event = false;
  for (int a = 0; a < 3; ++a) {
    if (spnav_poll_event(&ev) != 0) {
      have_event = true;
      break;
    }
    QThread::msleep(20);
  }

  if (!have_event) {
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, 0));
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, 0));
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, 0));
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3, 0));
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4, 0));
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5, 0));

    if (spnav_wait_event(&ev) == 0) {
      return false;
    }
  }

  do {
    if (ev.type == SPNAV_EVENT_MOTION) {
#ifdef DEBUG
      if ((ev.motion.x != 0) || (ev.motion.y != 0) || (ev.motion.z != 0)) {
        PRINTDB("Translate Event: x = %d, y = %d, z = %d", ev.motion.x % ev.motion.y % ev.motion.z);
      }
      if ((ev.motion.rx != 0) || (ev.motion.ry != 0) || (ev.motion.rz != 0)) {
        PRINTDB("Rotate Event: rx = %d, ry = %d, rz = %d", ev.motion.rx % ev.motion.ry % ev.motion.rz);
      }
#endif

      if (this->dominantAxisOnly) {
        // dominant axis only
        int m = ev.motion.x;
        if (abs(m) < abs(ev.motion.y)) m = ev.motion.y;
        if (abs(m) < abs(ev.motion.z)) m = ev.motion.z;
        if (abs(m) < abs(ev.motion.rx)) m = ev.motion.rx;
        if (abs(m) < abs(ev.motion.ry)) m = ev.motion.ry;
        if (abs(m) < abs(ev.motion.rz)) m = ev.motion.rz;

        if (ev.motion.x == m) {
          ev.motion.y = 0; ev.motion.z = 0; ev.motion.rx = 0; ev.motion.ry = 0; ev.motion.rz = 0;
        }
        if (ev.motion.y == m) {
          ev.motion.x = 0;                ev.motion.z = 0; ev.motion.rx = 0; ev.motion.ry = 0; ev.motion.rz = 0;
        }
        if (ev.motion.z == m) {
          ev.motion.x = 0; ev.motion.y = 0;                ev.motion.rx = 0; ev.motion.ry = 0; ev.motion.rz = 0;
        }
        if (ev.motion.rx == m) {
          ev.motion.x = 0; ev.motion.y = 0; ev.motion.z = 0;                 ev.motion.ry = 0; ev.motion.rz = 0;
        }
        if (ev.motion.ry == m) {
          ev.motion.x = 0; ev.motion.y = 0; ev.motion.z = 0; ev.motion.rx = 0;                 ev.motion.rz = 0;
        }
        if (ev.motion.rz == m) {
          ev.motion.x = 0; ev.motion.y = 0; ev.motion.z = 0; ev.motion.rx = 0; ev.motion.ry = 0;
        }
      }

      if (ev.motion.x != 0) {
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, ev.motion.x / 500.0));
      }
      if (ev.motion.y != 0) {
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, ev.motion.y / 500.0));
      }
      if (ev.motion.z != 0) {
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, ev.motion.z / 500.0));
      }
      if (ev.motion.rx != 0) {
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3, ev.motion.rx / 500.0));
      }
      if (ev.motion.ry != 0) {
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4, ev.motion.ry / 500.0));
      }
      if (ev.motion.rz != 0) {
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5, ev.motion.rz / 500.0));
      }
    } else if (ev.type == SPNAV_EVENT_BUTTON) {
      PRINTDB("Button Event: num = %d, %s", ev.button.bnum % (ev.button.press ? "pressed" : "released"));
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

void SpaceNavInputDriver::setDominantAxisOnly(bool var){
  this->dominantAxisOnly = var;
}

const std::string& SpaceNavInputDriver::get_name() const
{
  static std::string name = "SpaceNavInputDriver";
  return name;
}

std::string SpaceNavInputDriver::get_info() const
{
  return STR(get_name(), " ", (isOpen() ? "open" : "not open"), " ");
}
