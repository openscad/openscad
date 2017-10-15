/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2015 Clifford Wolf <clifford@clifford.at> and
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "InputDriverManager.h"
#include "JoystickInputDriver.h"

#include <linux/input.h>
#include <linux/joystick.h>

void JoystickInputDriver::run()
{
    struct js_event js;

    while (read(fd, &js, sizeof(struct js_event)) > 0) {
        switch (js.type & ~JS_EVENT_INIT) {
        case JS_EVENT_BUTTON:
            InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(js.number, js.value != 0));
            break;
        case JS_EVENT_AXIS:
            InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(js.number, js.value / 32767.0));
            break;
        }
    }
    ::close(fd);
}

JoystickInputDriver::JoystickInputDriver() : fd(-1)
{

}

JoystickInputDriver::~JoystickInputDriver()
{

}

bool JoystickInputDriver::open()
{
    fd = ::open("/dev/input/js0", O_RDONLY);
    if (fd < 0) {
        return false;
    }

    version = 0;
    ioctl(fd, JSIOCGVERSION, &version);
    if (version < 0x010000) {
        return false;
    }

    ioctl(fd, JSIOCGAXES, &axes);
    ioctl(fd, JSIOCGBUTTONS, &buttons);
    ioctl(fd, JSIOCGNAME(sizeof(name)), name);

    start();
    return true;
}

void JoystickInputDriver::close()
{

}

const std::string & JoystickInputDriver::get_name() const
{
    static std::string name = "JoystickInputDriver";
    return name;
}
