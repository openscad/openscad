/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2017 Clifford Wolf <clifford@clifford.at> and
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

#include "gui/input/QGamepadInputDriver.h"

#include "gui/input/InputDriverManager.h"

#include <string>

void QGamepadInputDriver::run()
{
}

QGamepadInputDriver::QGamepadInputDriver() : gamepad(nullptr)
{
}

bool QGamepadInputDriver::open()
{
  if (gamepad) {
    return false;
  }

  auto gamepads = QGamepadManager::instance()->connectedGamepads();
  if (gamepads.isEmpty()) {
    return false;
  }

  this->gamepad.reset(new QGamepad(*gamepads.begin(), this));

  connect(this->gamepad.get(), &QGamepad::axisLeftXChanged, this, [](double value){
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, value));
  });
  connect(this->gamepad.get(), &QGamepad::axisLeftYChanged, this, [](double value){
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, value));
  });
  connect(this->gamepad.get(), &QGamepad::axisRightXChanged, this, [](double value){
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, value));
  });
  connect(this->gamepad.get(), &QGamepad::axisRightYChanged, this, [](double value){
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3, value));
  });
  connect(this->gamepad.get(), &QGamepad::buttonL2Changed, this, [](double value){
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4, value));
  });
  connect(this->gamepad.get(), &QGamepad::buttonR2Changed, this, [](double value){
    InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5, value));
  });

  connect(this->gamepad.get(), &QGamepad::buttonAChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(0, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonBChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(1, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonXChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(2, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonYChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(3, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonL1Changed, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(4, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonR1Changed, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(5, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonSelectChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(6, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonStartChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(7, pressed));
  });

  connect(this->gamepad.get(), &QGamepad::buttonL3Changed, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(8, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonR3Changed, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(9, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonCenterChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(10, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonUpChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(11, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonDownChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(12, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonLeftChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(13, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonRightChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(14, pressed));
  });
  connect(this->gamepad.get(), &QGamepad::buttonGuideChanged, this, [](bool pressed){
    InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(15, pressed));
  });

  return true;
}

void QGamepadInputDriver::close()
{
  gamepad.reset();
}

const std::string& QGamepadInputDriver::get_name() const
{
  static std::string name = "QGamepadInputDriver";
  return name;
}

bool QGamepadInputDriver::isOpen() const
{
  return this->gamepad ? this->gamepad->isConnected() : false;
}

std::string QGamepadInputDriver::get_info() const
{
  const auto status = isOpen()
      ? std::string{"connected: "} + this->gamepad->name().toUtf8().constData()
      : std::string{"not connected"};

  return get_name() + " " + status;
}
