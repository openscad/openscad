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
#pragma once

#include <QTimer>
#include <QObject>

#include <cstddef>
#include <string>
#include "core/Settings.h"
#include "gui/input/InputDriverEvent.h"

class InputEventMapper : public QObject, public InputEventHandler
{
  Q_OBJECT

public:
  constexpr static inline size_t getMaxButtons() { return Settings::max_buttons; }
  constexpr static inline size_t getMaxAxis() { return Settings::max_axis; }

private:
  QTimer *timer;
  double axisRawValue[Settings::max_axis];
  double axisTrimValue[Settings::max_axis];
  double axisDeadzone[Settings::max_axis];
  QString actions[Settings::max_buttons];
  int translate[6];
  int rotate[6];
  int zoom;
  int zoom2;
  volatile bool stopRequest;

  double scale(double val);
  double getAxisValue(int config);
  int parseSettingValue(const std::string& val);
  bool generateDeferredEvents();
  void considerGeneratingDeferredEvents();
  bool button_state[Settings::max_buttons];
  bool button_state_last[Settings::max_buttons];

  static InputEventMapper *self;

  double translationGain;
  double translationVPRelGain;
  double rotateGain;
  double rotateVPRelGain;
  double zoomGain;

public:
  InputEventMapper();

  void stop();

  void onAxisChanged(class InputEventAxisChanged *event) override;
  void onButtonChanged(class InputEventButtonChanged *event) override;

  void onTranslateEvent(class InputEventTranslate *event) override;
  void onRotateEvent(class InputEventRotate *event) override;
  void onRotate2Event(class InputEventRotate2 *event) override;
  void onActionEvent(class InputEventAction *event) override;
  void onZoomEvent(class InputEventZoom *event) override;

  void onInputMappingUpdated();
  void onInputCalibrationUpdated();
  void onInputGainUpdated();

  void onAxisAutoTrim();
  void onAxisTrimReset();

  static InputEventMapper *instance();

  static Settings::SettingsEntryString& inputButtonSettings(size_t id);
  static Settings::SettingsEntryDouble& axisTrimSettings(size_t id);
  static Settings::SettingsEntryDouble& axisDeadzoneSettings(size_t id);

private slots:
  void onTimer();
};
