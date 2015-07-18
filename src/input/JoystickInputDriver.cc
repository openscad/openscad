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
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "InputDriverManager.h"
#include "JoystickInputDriver.h"

#include <linux/input.h>
#include <linux/joystick.h>

void JoystickInputDriver::run()
{
    int *axis;
    char *button;
    struct js_event js;

    axis = (int *) calloc(axes, sizeof(int));
    button = (char *) calloc(buttons, sizeof(char));

    while (1) {
        timespec start;
        clock_gettime(CLOCK_REALTIME, &start);
        while (read(fd, &js, sizeof(struct js_event)) > 0) {
            switch (js.type & ~JS_EVENT_INIT) {
            case JS_EVENT_BUTTON:
                button[js.number] = js.value;
                if (!(js.type & JS_EVENT_INIT)) {
                    if (js.number == 0) {
                        InputDriverManager::instance()->postEvent(new InputEventButton(js.number, js.value));
                    }
                }
                break;
            case JS_EVENT_AXIS:
                axis[js.number] = js.value;
                break;
            }
        }
        if (errno != EAGAIN) {
            ::close(fd);
            return;
        }

        timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        unsigned long time = 30000 - (now.tv_nsec - start.tv_nsec) / 1000;
        if (time < 5000) {
            time = 5000;
        }
        usleep(time);

        const double threshold = 0.3;

        if (button[1]) {
            double zoom = calc(axis[5]);
            if (fabs(zoom) > 0.5) {
                InputDriverManager::instance()->postEvent(new InputEventZoom(50 * zoom));
            }
            continue;
        }

        double rx = calc(axis[3]);
        double ry = calc(axis[4]);
        double rz = calc(axis[5]);
        if ((fabs(rx) > threshold) || (fabs(ry) > threshold) || (fabs(rz) > threshold)) {
            InputDriverManager::instance()->postEvent(new InputEventRotate(rx, -ry, -rz));
        }

        double tx = calc(axis[0]);
        double ty = calc(axis[1]);
        double tz = calc(axis[2]);
        if ((fabs(tx) > threshold) || (fabs(ty) > threshold) || (fabs(tz) > threshold)) {
            InputDriverManager::instance()->postEvent(new InputEventTranslate(tx, -ty, -tz));
        }
    }
}

double JoystickInputDriver::calc(double val)
{
    double x = val / 8000.0;
    double xx = x < 0 ? -exp(-x) + 1 : exp(x) - 1;
    return xx / 6.0;
}

JoystickInputDriver::JoystickInputDriver() : fd(-1)
{

}

JoystickInputDriver::~JoystickInputDriver()
{

}

bool JoystickInputDriver::open()
{
    fd = ::open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
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
