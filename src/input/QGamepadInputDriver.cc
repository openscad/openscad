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

#include "InputDriverManager.h"
#include "QGamepadInputDriver.h"

void QGamepadInputDriver::run()
{
}

QGamepadInputDriver::QGamepadInputDriver() : gamepad(nullptr)
{
}

QGamepadInputDriver::~QGamepadInputDriver()
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

    this->gamepad = new QGamepad(*gamepads.begin(), this);

    connect(this->gamepad, &QGamepad::axisLeftXChanged, this, [](double value){
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, value));
    });
    connect(this->gamepad, &QGamepad::axisLeftYChanged, this, [](double value){
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, value));
    });
    connect(this->gamepad, &QGamepad::axisRightXChanged, this, [](double value){
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, value));
    });
    connect(this->gamepad, &QGamepad::axisRightYChanged, this, [](double value){
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3, value));
    });
    connect(this->gamepad, &QGamepad::buttonL2Changed, this, [](double value){
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4, value));
    });
    connect(this->gamepad, &QGamepad::buttonR2Changed, this, [](double value){
        InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5, value));
    });

    connect(this->gamepad, &QGamepad::buttonAChanged, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(0, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonBChanged, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(1, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonXChanged, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(2, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonYChanged, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(3, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonL1Changed, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(4, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonR1Changed, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(5, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonSelectChanged, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(6, pressed));
    });
    connect(this->gamepad, &QGamepad::buttonStartChanged, this, [](bool pressed){
		InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(7, pressed));
    });

	return true;
}

void QGamepadInputDriver::close()
{
	delete this->gamepad;
	this->gamepad = nullptr;
}

const std::string & QGamepadInputDriver::get_name() const
{
    static std::string name = "QGamepadInputDriver";
    return name;
}
