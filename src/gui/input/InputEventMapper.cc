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
#include "gui/input/InputEventMapper.h"
#include "gui/input/InputDriverManager.h"
#include "gui/Settings.h"
#include "gui/Preferences.h"
#include "gui/input/AxisConfigWidget.h"
#include "gui/input/ButtonConfigWidget.h"
#include <QMetaObject>
#include <QTimer>
#include <cstddef>
#include <string>
#include <cmath>
#include <QSettings>

InputEventMapper *InputEventMapper::self = nullptr;

InputEventMapper::InputEventMapper()
{
  stopRequest = false;

  for (size_t a = 0; a < InputDriver::max_axis; ++a) {
    axisRawValue[a] = 0.0;
    axisTrimValue[a] = 0.0;
    axisDeadzone[a] = 0.1;
  }
  for (size_t a = 0; a < InputDriver::max_buttons; ++a) {
    button_state[a] = false;
    button_state_last[a] = false;
  }

  translationGain = 1.00;
  translationVPRelGain = 1.00;
  rotateGain = 1.00;
  rotateVPRelGain = 1.00;
  zoomGain = 1.00;

  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
  timer->start(30);

  onInputMappingUpdated();
  onInputCalibrationUpdated();
  onInputGainUpdated();

  self = this;
}

InputEventMapper *InputEventMapper::instance()
{
  if (!self) {
    self = new InputEventMapper();
  }
  return self;
}

size_t InputEventMapper::getMaxButtons(){
  return InputDriver::max_buttons;
}

size_t InputEventMapper::getMaxAxis(){
  return InputDriver::max_axis;
}

/*
    -1 -> 0.196
    0 -> 0
    1 ->  10.72
 */
double InputEventMapper::scale(double val)
{
  double x = 4 * val;
  double xx = x < 0 ? -exp(-x) + 1 : exp(x) - 1;
  return xx / 5.0;
}

double InputEventMapper::getAxisValue(int config)
{
  if (config == 0)    // avoid indexing by -1 when using default settings (and causing bizarre behavior)
    return scale(0);

  int idx = abs(config) - 1;
  if (idx > 8)        // avoid reading over end of arrays (and causing segfaults)
    return scale(0);

  bool neg = config < 0;
  double trimmedVal = axisRawValue[idx] + axisTrimValue[idx];
  double val = neg ? -trimmedVal : trimmedVal;
  if (val < axisDeadzone[idx] and - val < axisDeadzone[idx]) {
    val = 0;
  }
  return scale(val);
}

bool InputEventMapper::generateDeferredEvents()
{
  bool any = false;
  const double threshold = 0.01;

  double tx = getAxisValue(translate[0]) * translationGain;
  double ty = getAxisValue(translate[1]) * translationGain;
  double tz = getAxisValue(translate[2]) * translationGain;
  if ((fabs(tx) > threshold) || (fabs(ty) > threshold) || (fabs(tz) > threshold)) {
    InputEvent *inputEvent = new InputEventTranslate(tx, ty, tz);
    InputDriverManager::instance()->postEvent(inputEvent);
    any = true;
  }

  double txVPRel = getAxisValue(translate[3]) * translationVPRelGain;
  double tyVPRel = getAxisValue(translate[4]) * translationVPRelGain;
  double tzVPRel = getAxisValue(translate[5]) * translationVPRelGain;
  if ((fabs(txVPRel) > threshold) || (fabs(tyVPRel) > threshold) || (fabs(tzVPRel) > threshold)) {
    InputEvent *inputEvent = new InputEventTranslate(txVPRel, tyVPRel, tzVPRel, true, true, false);
    InputDriverManager::instance()->postEvent(inputEvent);
    any = true;
  }

  double rx = getAxisValue(rotate[0]) * rotateGain;
  double ry = getAxisValue(rotate[1]) * rotateGain;
  double rz = getAxisValue(rotate[2]) * rotateGain;
  if ((fabs(rx) > threshold) || (fabs(ry) > threshold) || (fabs(rz) > threshold)) {
    InputEvent *inputEvent = new InputEventRotate(rx, ry, rz);
    InputDriverManager::instance()->postEvent(inputEvent);
    any = true;
  }

  double rxVPRel = getAxisValue(rotate[3]) * rotateVPRelGain;
  double ryVPRel = getAxisValue(rotate[4]) * rotateVPRelGain;
  double rzVPRel = getAxisValue(rotate[5]) * rotateVPRelGain;
  if ((fabs(rxVPRel) > threshold) || (fabs(ryVPRel) > threshold) || (fabs(rzVPRel) > threshold)) {
    InputEvent *inputEvent = new InputEventRotate2(rxVPRel, ryVPRel, rzVPRel);
    InputDriverManager::instance()->postEvent(inputEvent);
    any = true;
  }

  double z = (getAxisValue(zoom) + getAxisValue(zoom2)) * zoomGain;
  if (fabs(z) > threshold) {
    InputEvent *inputEvent = new InputEventZoom(z);
    InputDriverManager::instance()->postEvent(inputEvent);
    any = true;
  }

  return any;
}

void InputEventMapper::considerGeneratingDeferredEvents()
{
  if (!timer->isActive()) {
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 30));
  }
}

void InputEventMapper::onTimer()
{
  bool generated_any_events = generateDeferredEvents();

  //update the UI on time, NOT on event as a joystick can fire a high rate of events
  for (size_t i = 0; i < InputDriver::max_buttons; ++i) {
    if (button_state[i] != button_state_last[i]) {
      button_state_last[i] = button_state[i];
      Preferences::inst()->ButtonConfig->updateButtonState(i, button_state[i]);
    }
  }
  for (size_t i = 0; i < InputDriver::max_axis; ++i) {
    Preferences::inst()->AxisConfig->AxesChanged(i, axisRawValue[i] + axisTrimValue[i]);
  }

  if (!generated_any_events) {
    // the current axis positions do not generate input events,
    // so we can stop the polling which is used to to generate them
    timer->stop();
  }
}

void InputEventMapper::onAxisChanged(InputEventAxisChanged *event)
{
  axisRawValue[event->axis] = event->value;
  considerGeneratingDeferredEvents();
}

void InputEventMapper::onButtonChanged(InputEventButtonChanged *event)
{
  unsigned int button = event->button;

  if (button < InputDriver::max_buttons) {
    if (event->down) {
      this->button_state[button] = true;
    } else {
      this->button_state[button] = false;
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
  considerGeneratingDeferredEvents();
}

void InputEventMapper::onTranslateEvent(InputEventTranslate *event)
{
  InputDriverManager::instance()->postEvent(event);
}

void InputEventMapper::onRotateEvent(InputEventRotate *event)
{
  InputDriverManager::instance()->postEvent(event);
}

void InputEventMapper::onRotate2Event(InputEventRotate2 *event)
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

int InputEventMapper::parseSettingValue(const std::string& val)
{
  if (val.length() != 2) {
    return 0;
  }
  return atoi(val.c_str());
}

void InputEventMapper::onInputMappingUpdated()
{
  for (size_t i = 0; i < InputDriver::max_buttons; ++i) {
    actions[i] = QString::fromStdString(Settings::Settings::inputButton(i).value());
  }

  translate[0] = parseSettingValue(Settings::Settings::inputTranslationX.value());
  translate[1] = parseSettingValue(Settings::Settings::inputTranslationY.value());
  translate[2] = parseSettingValue(Settings::Settings::inputTranslationZ.value());
  translate[3] = parseSettingValue(Settings::Settings::inputTranslationXVPRel.value());
  translate[4] = parseSettingValue(Settings::Settings::inputTranslationYVPRel.value());
  translate[5] = parseSettingValue(Settings::Settings::inputTranslationZVPRel.value());
  rotate[0] = parseSettingValue(Settings::Settings::inputRotateX.value());
  rotate[1] = parseSettingValue(Settings::Settings::inputRotateY.value());
  rotate[2] = parseSettingValue(Settings::Settings::inputRotateZ.value());
  rotate[3] = parseSettingValue(Settings::Settings::inputRotateXVPRel.value());
  rotate[4] = parseSettingValue(Settings::Settings::inputRotateYVPRel.value());
  rotate[5] = parseSettingValue(Settings::Settings::inputRotateZVPRel.value());
  zoom = parseSettingValue(Settings::Settings::inputZoom.value());
  zoom2 = parseSettingValue(Settings::Settings::inputZoom2.value());
  considerGeneratingDeferredEvents();
}

void InputEventMapper::onInputGainUpdated()
{
  translationGain = Settings::Settings::inputTranslationGain.value();
  translationVPRelGain = Settings::Settings::inputTranslationVPRelGain.value();
  rotateGain = Settings::Settings::inputRotateGain.value();
  rotateVPRelGain = Settings::Settings::inputRotateVPRelGain.value();
  zoomGain = Settings::Settings::inputZoomGain.value();

  considerGeneratingDeferredEvents();
}

void InputEventMapper::onInputCalibrationUpdated()
{
  for (size_t i = 0; i < InputDriver::max_axis; ++i) {
    axisTrimValue[i] = Settings::Settings::axisTrim(i).value();
    axisDeadzone[i] = Settings::Settings::axisDeadzone(i).value();
  }
  considerGeneratingDeferredEvents();
}

void InputEventMapper::onAxisAutoTrim()
{
  for (size_t i = 0; i < InputDriver::max_axis; ++i) {
    axisTrimValue[i] = -axisRawValue[i];
    Settings::Settings::axisTrim(i).setValue(axisTrimValue[i]);
  }
  considerGeneratingDeferredEvents();
}

void InputEventMapper::onAxisTrimReset()
{
  for (size_t i = 0; i < InputDriver::max_axis; ++i) {
    axisTrimValue[i] = 0.00;
    Settings::Settings::axisTrim(i).setValue(axisTrimValue[i]);
  }
  considerGeneratingDeferredEvents();
}

void InputEventMapper::stop(){
  stopRequest = true;
  timer->stop();
}
