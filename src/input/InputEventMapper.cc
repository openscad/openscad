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
#include "Preferences.h"
#include "AxisConfigWidget.h"
#include "ButtonConfigWidget.h"

#include <math.h>
#include <QSettings>

InputEventMapper * InputEventMapper::self = 0;

InputEventMapper::InputEventMapper()
{
    stopRequest=false;

    for (int a = 0;a < max_axis;a++) {
        axisRawValue[a] = 0.0;
        axisTrimValue[a] = 0.0;
        axisDeadzone[a] = 0.1;
    }
    for (int a = 0;a < max_buttons;a++) {
        button_state[a]=false;
        button_state_last[a]=false;
    }

    translationGain=1.00;
    translationVPRelGain=1.00;
    rotateGain=1.00;
    zoomGain=1.00;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(30);

    onInputMappingUpdated();
    onInputCalibrationUpdated();
    onInputGainUpdated();

    self=this;
}

InputEventMapper::~InputEventMapper()
{

}

InputEventMapper * InputEventMapper::instance()
{
    if (!self) {
        self = new InputEventMapper();
    }
    return self;
}

int InputEventMapper::getMaxButtons(){
    return max_buttons;
}

int InputEventMapper::getMaxAxis(){
    return max_axis;
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
    double trimmedVal = axisRawValue[idx] + axisTrimValue[idx];
    double val = neg ? -trimmedVal : trimmedVal;
    if(val < axisDeadzone[idx] and -val < axisDeadzone[idx]){
        val=0;
    }
    return scale(val);
}

void InputEventMapper::onTimer()
{
    const double threshold = 0.01;

    double tx = getAxisValue(translate[0])*translationGain;
    double ty = getAxisValue(translate[1])*translationGain;
    double tz = getAxisValue(translate[2])*translationGain;
    if ((fabs(tx) > threshold) || (fabs(ty) > threshold) || (fabs(tz) > threshold)) {
        InputEvent *inputEvent = new InputEventTranslate(tx, ty, tz);
        InputDriverManager::instance()->postEvent(inputEvent);
    }
    
    double txVPRel = getAxisValue(translate[3])*translationVPRelGain;
    double tyVPRel = getAxisValue(translate[4])*translationVPRelGain;
    double tzVPRel = getAxisValue(translate[5])*translationVPRelGain;
    if ((fabs(txVPRel) > threshold) || (fabs(tyVPRel) > threshold) || (fabs(tzVPRel) > threshold)) {
        InputEvent *inputEvent = new InputEventTranslate(txVPRel, tyVPRel, tzVPRel, true, true, false);
        InputDriverManager::instance()->postEvent(inputEvent);
    }
    
    double rx = getAxisValue(rotate[0])*rotateGain;
    double ry = getAxisValue(rotate[1])*rotateGain;
    double rz = getAxisValue(rotate[2])*rotateGain;
    if ((fabs(rx) > threshold) || (fabs(ry) > threshold) || (fabs(rz) > threshold)) {
        InputEvent *inputEvent = new InputEventRotate(rx, ry, rz);
        InputDriverManager::instance()->postEvent(inputEvent);
    }

    double z = (getAxisValue(zoom)+getAxisValue(zoom2))*zoomGain;
    if (fabs(z) > threshold) {
        InputEvent *inputEvent = new InputEventZoom(z);
        InputDriverManager::instance()->postEvent(inputEvent);
    }
    
    //update the UI on time, NOT on event as a joystick can fire a high rate of events
    for (int i = 0; i < max_buttons; i++ ){
        if(button_state[i] != button_state_last[i]){
            button_state_last[i] = button_state[i];
            Preferences::inst()->ButtonConfig->updateButtonState(i,button_state[i]);
        }
    }
    for (int i = 0; i < max_axis; i++ ){ 
       Preferences::inst()->AxisConfig->AxesChanged(i,axisRawValue[i] + axisTrimValue[i]);
    }

}

void InputEventMapper::onAxisChanged(InputEventAxisChanged *event)
{
    axisRawValue[event->axis] = event->value;
}

void InputEventMapper::onButtonChanged(InputEventButtonChanged *event)
{
    int button = event->button;

    if (button < max_buttons) {
        if (event->down) {
            this->button_state[button]=true;
        }else{
            this->button_state[button]=false;
        }

        if (!event->down) {
            return;
        }

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
    for (int i = 0; i < max_buttons; i++ ){
		std::string is = std::to_string(i);
		Settings::SettingsEntry* ent =s->getSettingEntryByName("button" +is);
		actions[i] =QString::fromStdString(s->get(*ent).toString());
	}
    
    translate[0] = parseSettingValue(s->get(Settings::Settings::inputTranslationX).toString());
    translate[1] = parseSettingValue(s->get(Settings::Settings::inputTranslationY).toString());
    translate[2] = parseSettingValue(s->get(Settings::Settings::inputTranslationZ).toString());
    translate[3] = parseSettingValue(s->get(Settings::Settings::inputTranslationXVPRel).toString());
    translate[4] = parseSettingValue(s->get(Settings::Settings::inputTranslationYVPRel).toString());
    translate[5] = parseSettingValue(s->get(Settings::Settings::inputTranslationZVPRel).toString());
    rotate[0] = parseSettingValue(s->get(Settings::Settings::inputRotateX).toString());
    rotate[1] = parseSettingValue(s->get(Settings::Settings::inputRotateY).toString());
    rotate[2] = parseSettingValue(s->get(Settings::Settings::inputRotateZ).toString());
    zoom = parseSettingValue(s->get(Settings::Settings::inputZoom).toString());
    zoom2 = parseSettingValue(s->get(Settings::Settings::inputZoom2).toString());
}

void InputEventMapper::onInputGainUpdated()
{
    Settings::Settings *s = Settings::Settings::inst();

    translationGain = (double)s->get(Settings::Settings::inputTranslationGain).toDouble();

    translationVPRelGain = (double)s->get(Settings::Settings::inputTranslationVPRelGain).toDouble();

    rotateGain = (double)s->get(Settings::Settings::inputRotateGain).toDouble();

    zoomGain = (double)s->get(Settings::Settings::inputZoomGain).toDouble();
}

void InputEventMapper::onInputCalibrationUpdated()
{
    for (int a = 0;a < max_axis;a++) {
        std::string s = std::to_string(a);
        Settings::Settings *setting = Settings::Settings::inst();
        Settings::SettingsEntry* ent;

        ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" + s );
        if(ent != nullptr){
            axisTrimValue[a] = (double)setting->get(*ent).toDouble();
        }
        ent = Settings::Settings::inst()->getSettingEntryByName("axisDeadzone" + s );
        if(ent != nullptr){
            axisDeadzone[a] = (double)setting->get(*ent).toDouble();
        }
    }
}

void InputEventMapper::onAxisAutoTrim()
{
    Settings::Settings *s = Settings::Settings::inst();
    for (int i = 0; i < max_axis; i++ ){ 
        std::string is = std::to_string(i);
        axisTrimValue[i] = -axisRawValue[i];
        Settings::SettingsEntry* ent =s->getSettingEntryByName("axisTrim" +is);
        s->set(*ent, axisTrimValue[i]);
    }
}

void InputEventMapper::onAxisTrimReset()
{
    Settings::Settings *s = Settings::Settings::inst();
    for (int i = 0; i < max_axis; i++ ){ 
        std::string is = std::to_string(i);
        axisTrimValue[i] = 0.00;
        Settings::SettingsEntry* ent =s->getSettingEntryByName("axisTrim" +is);
        s->set(*ent, axisTrimValue[i]);
    }
}

void InputEventMapper::stop(){
    stopRequest=true;
    timer->stop();
}
