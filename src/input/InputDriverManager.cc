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

InputDriverManager::InputDriverManager()
{
    currentWindow = 0;
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), this, SLOT(onFocusChanged(QWidget *, QWidget *)));
    timer = new QTimer(this);
}

InputDriverManager::~InputDriverManager()
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

void InputDriverManager::registerActions(const QList<QAction *> &actions, const int level)
{
    foreach(QAction *action, actions) {
        if (!action->objectName().isEmpty()) {
            for (int a = 0;a < level;a++) {
                printf("  ");
            }
            printf("%s - %s - %d\n",
                action->toolTip().toStdString().c_str(),
                action->objectName().toStdString().c_str(),
                action->menu() ? action->menu()->actions().size() : 0);
        }
        if (action->menu()) {
            registerActions(action->menu()->actions(), level + 1);
        }
    }
}

void InputDriverManager::init()
{
    doOpen(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(10 * 1000);
}

void InputDriverManager::onTimeout()
{
    for (drivers_t::iterator it = drivers.begin(); it != drivers.end(); it++) {
        InputDriver *driver = (*it);
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
    for (drivers_t::iterator it = drivers.begin();it != drivers.end();it++) {
        InputDriver *driver = (*it);
        if (driver->openOnce()) {
            continue;
        }
        if (driver->open()) {
            break;
        }
    }

    if (firstOpen) {
        for (drivers_t::iterator it = drivers.begin();it != drivers.end();it++) {
            InputDriver *driver = (*it);
            if (driver->openOnce()) {
                driver->open();
            }
        }
    }
}

std::string InputDriverManager::listDrivers()
{
    std::stringstream stream;
    const char *sep = "";
    for (drivers_t::iterator it = drivers.begin();it != drivers.end();it++) {
        InputDriver *driver = (*it);
        stream << sep << driver->get_name();
        if (driver->isOpen()) {
            stream << "*";
        }
        sep = ", ";
    }
    return stream.str();
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

void InputDriverManager::onFocusChanged(QWidget *, QWidget *current)
{
    if (current) {
        currentWindow = dynamic_cast<MainWindow *>(current->window());
    }
}
