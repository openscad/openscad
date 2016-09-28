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
#include "settings.h"

#include <math.h>
#include <QSettings>

InputEventMapper::InputEventMapper()
{
    for (int a = 0;a < 10;a++) {
        axisValue[a] = 0;
    }

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(30);

    onInputMappingUpdated();
}

InputEventMapper::~InputEventMapper()
{

}

double InputEventMapper::scale(double val)
{
    double x = 4 * val;
    double xx = x < 0 ? -exp(-x) + 1 : exp(x) - 1;
    return xx / 5.0;
}

double InputEventMapper::getAxisValue(int config)
{
    int idx = abs(config) - 1;
    bool neg = config < 0;
    double val = neg ? -axisValue[idx] : axisValue[idx];
    return scale(val);
}

void InputEventMapper::onTimer()
{
    const double threshold = 0.1;

    double tx = getAxisValue(translate[0]);
    double ty = getAxisValue(translate[1]);
    double tz = getAxisValue(translate[2]);
    if ((fabs(tx) > threshold) || (fabs(ty) > threshold) || (fabs(tz) > threshold)) {
        InputEvent *inputEvent = new InputEventTranslate(tx, ty, tz);
        InputDriverManager::instance()->postEvent(inputEvent);
    }

    double rx = getAxisValue(rotate[0]);
    double ry = getAxisValue(rotate[1]);
    double rz = getAxisValue(rotate[2]);
    if ((fabs(rx) > threshold) || (fabs(ry) > threshold) || (fabs(rz) > threshold)) {
        InputEvent *inputEvent = new InputEventRotate(rx, ry, rz);
        InputDriverManager::instance()->postEvent(inputEvent);
    }

    double z = getAxisValue(zoom);
    if (fabs(z) > threshold) {
        InputEvent *inputEvent = new InputEventZoom(z);
        InputDriverManager::instance()->postEvent(inputEvent);
    }
}

void InputEventMapper::onAxisChanged(InputEventAxisChanged *event)
{
    axisValue[event->axis] = event->value;
}

void InputEventMapper::onButtonChanged(InputEventButtonChanged *event)
{
    if (!event->down) {
        return;
    }

    if (event->button < 10) {
        std::string action = actions[event->button].toStdString();
        if (!action.empty()) {
            InputEvent *inputEvent = new InputEventAction(action, false);
            InputDriverManager::instance()->postEvent(inputEvent);
        }
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

int InputEventMapper::parseSettingValue(const std::string val)
{
    if (val.length() != 2) {
        return 0;
    }
    return atoi(val.c_str());
}

void InputEventMapper::onInputMappingUpdated()
{
    Settings::Settings *s = Settings::Settings::inst();

    actions[0] = QString(s->get(Settings::Settings::inputButton1).toString().c_str());
    actions[1] = QString(s->get(Settings::Settings::inputButton2).toString().c_str());
    actions[2] = QString(s->get(Settings::Settings::inputButton3).toString().c_str());
    actions[3] = QString(s->get(Settings::Settings::inputButton4).toString().c_str());
    actions[4] = QString(s->get(Settings::Settings::inputButton5).toString().c_str());
    actions[5] = QString(s->get(Settings::Settings::inputButton6).toString().c_str());
    actions[6] = QString(s->get(Settings::Settings::inputButton7).toString().c_str());
    actions[7] = QString(s->get(Settings::Settings::inputButton8).toString().c_str());

    translate[0] = parseSettingValue(s->get(Settings::Settings::inputTranslationX).toString());
    translate[1] = parseSettingValue(s->get(Settings::Settings::inputTranslationY).toString());
    translate[2] = parseSettingValue(s->get(Settings::Settings::inputTranslationZ).toString());
    rotate[0] = parseSettingValue(s->get(Settings::Settings::inputRotateX).toString());
    rotate[1] = parseSettingValue(s->get(Settings::Settings::inputRotateY).toString());
    rotate[2] = parseSettingValue(s->get(Settings::Settings::inputRotateZ).toString());
    zoom = parseSettingValue(s->get(Settings::Settings::inputZoom).toString());
}
