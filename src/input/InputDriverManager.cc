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
#include "InputDriverManager.h"

#include "printutils.h"

InputDriverManager * InputDriverManager::self = 0;

/**
 * This can be called from non-GUI context, so no Qt initialization is done
 * at this point.
 */
InputDriverManager::InputDriverManager(void) : currentWindow(nullptr), timer(nullptr)
{
}

InputDriverManager::~InputDriverManager(void)
{

}

InputDriverManager * InputDriverManager::instance()
{
    if (!self) {
        self = new InputDriverManager();
    }
    return self;
}

void InputDriverManager::registerDriver(InputDriver *driver)
{
    this->drivers.push_back(driver);
}

void InputDriverManager::unregisterDriver(InputDriver *driver)
{
    this->drivers.remove(driver);
}

void InputDriverManager::registerActions(const QList<QAction *> &actions, const QString parent)
{
	for (const auto action : actions) {
		const auto description = parent + action->text();
		if (!action->objectName().isEmpty()) {
			this->actions.push_back({action->objectName(), description, action->icon()});
		}
		if (action->menu()) {
			registerActions(action->menu()->actions(), description +  QString::fromUtf8(u8" \u2192 "));
		}
	}
}

void InputDriverManager::init()
{
    timer = new QTimer(this);
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), this, SLOT(onFocusChanged(QWidget *, QWidget *)));

    doOpen(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(10 * 1000);
}

void InputDriverManager::onTimeout()
{
    for (auto driver : drivers) {
        if (driver->openOnce()) {
            continue;
        }
        if (driver->isOpen()) {
            return;
        }
    }
    doOpen(false);
}

void InputDriverManager::doOpen(bool firstOpen)
{
    for (auto driver : drivers) {
        if (driver->openOnce()) {
            continue;
        }
        if (driver->open()) {
            break;
        }
    }

    if (firstOpen) {
        for (auto driver : drivers) {
            if (driver->openOnce()) {
                driver->open();
            }
        }
    }
}

std::string InputDriverManager::listDrivers() const
{
    std::ostringstream stream;
    const char *sep = "";
    for (auto driver : drivers) {
        stream << sep << driver->get_name();
        if (driver->isOpen()) {
            stream << "*";
        }
        sep = ", ";
    }
    return stream.str();
}

std::string InputDriverManager::listDriverInfos() const
{
    std::ostringstream stream;
    const char *sep = "";
    for (auto driver : drivers) {
        stream << sep << driver->get_info();
        sep = "\n";
    }
    return stream.str();
}

void InputDriverManager::closeDrivers()
{
    if (timer != nullptr) {
        timer->stop();
    }
    InputEventMapper::instance()->stop();

    for (auto driver : drivers) {
        driver->close();
    }
}

void InputDriverManager::sendEvent(InputEvent *event)
{
    event->deliver(&mapper);
}

void InputDriverManager::postEvent(InputEvent *event)
{
    QWidget *window = event->activeOnly ? QApplication::activeWindow() : currentWindow;
    if (window) {
        QCoreApplication::postEvent(window, event);
    }
}

const std::list<ActionStruct> & InputDriverManager::getActions() const
{
    return actions;
}

QList<double> InputDriverManager::getTranslation() const
{
    const MainWindow *window = currentWindow;
    if (window) {
        return window->getTranslation();
    }
    return QList<double>({0.0, 0.0, 0.0});
}

QList<double> InputDriverManager::getRotation() const
{
    const MainWindow *window = currentWindow;
    if (window) {
        return window->getRotation();
    }
    return QList<double>({0.0, 0.0, 0.0});
}

void InputDriverManager::setCurrentWindow(QWidget *current)
{
    if (current) {
        currentWindow = dynamic_cast<MainWindow *>(current->window());
    }
}

void InputDriverManager::onFocusChanged(QWidget *, QWidget *current)
{
    if (current) {
        currentWindow = dynamic_cast<MainWindow *>(current->window());
    }
}

void InputDriverManager::onInputMappingUpdated()
{
    mapper.onInputMappingUpdated();
}

void InputDriverManager::onInputCalibrationUpdated()
{
    mapper.onInputCalibrationUpdated();
}

void InputDriverManager::onInputGainUpdated()
{
    mapper.onInputGainUpdated();
}

int InputDriverManager::getButtonCount(){
    int max=0;
    for (auto driver : drivers) {
        if(driver->isOpen()){
            max = std::max(max, driver->getButtonCount());
        }
    }
    return max;
}

int InputDriverManager::getAxisCount(){
    int max=0;
    for (auto driver : drivers) {
        if(driver->isOpen()){
            max = std::max(max, driver->getAxisCount());
        }
    }
    return max;
}
