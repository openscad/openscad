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
#include "InputEventMapper.h"
#include "InputDriverManager.h"

#include <math.h>

InputEventMapper::InputEventMapper()
{
    for (int a = 0;a < 10;a++) {
        axis[a] = 0;
    }

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(30);
}

InputEventMapper::~InputEventMapper()
{

}

double InputEventMapper::scale(double val)
{
    double x = 4 * val;
    double xx = x < 0 ? -exp(-x) + 1 : exp(x) - 1;
    return xx / 6.0;
}

void InputEventMapper::onTimer()
{
    const double threshold = 0.1;

    double tx = scale(axis[0]);
    double ty = -scale(axis[1]);
    double tz = -scale(axis[2]);
    if ((fabs(tx) > threshold) || (fabs(ty) > threshold) || (fabs(tz) > threshold)) {
        InputEvent *inputEvent = new InputEventTranslate(tx, ty, tz);
        InputDriverManager::instance()->postEvent(inputEvent);
    }

    double rx = scale(axis[3]);
    double ry = -scale(axis[4]);
    double rz = -scale(axis[5]);
    if ((fabs(rx) > threshold) || (fabs(ry) > threshold) || (fabs(rz) > threshold)) {
        InputEvent *inputEvent = new InputEventRotate(rx, ry, rz);
        InputDriverManager::instance()->postEvent(inputEvent);
    }
}

void InputEventMapper::onAxisChanged(InputEventAxisChanged *event)
{
    axis[event->axis] = event->value;
}

void InputEventMapper::onButtonChanged(InputEventButtonChanged *event)
{
    if (!event->down) {
        return;
    }

    InputEvent *inputEvent = 0;

    switch (event->button) {
    case 0:
        inputEvent = new InputEventAction("viewActionResetView");
        break;
    case 1:
        inputEvent = new InputEventAction("viewActionViewAll");
        break;
    }

    if (inputEvent) {
        InputDriverManager::instance()->postEvent(inputEvent);
    }
}

void InputEventMapper::onTranslateEvent(InputEventTranslate *event)
{
    InputDriverManager::instance()->postEvent(event);
}

void InputEventMapper::onRotateEvent(InputEventRotate *event)
{
    InputDriverManager::instance()->postEvent(event);
}

void InputEventMapper::onActionEvent(InputEventAction *event)
{
    InputDriverManager::instance()->postEvent(event);
}

void InputEventMapper::onZoomEvent(InputEventZoom *event)
{
    InputDriverManager::instance()->postEvent(event);
}
