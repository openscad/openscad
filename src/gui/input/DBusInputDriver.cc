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
#include "gui/input/DBusInputDriver.h"
#include <QDBusConnection>
#include <QList>
#include <QStringList>
#include "gui/input/InputDriverManager.h"
#include "utils/printutils.h"
#include "openscad_adaptor.h"
#include "openscad_interface.h"

void DBusInputDriver::run()
{

}

DBusInputDriver::DBusInputDriver()
{
  name = "DBusInputDriver";
}

bool DBusInputDriver::openOnce() const
{
  return true;
}

bool DBusInputDriver::isOpen() const
{
  return is_open;
}

bool DBusInputDriver::open()
{
  if (is_open) {
    return true;
  }

  QDBusConnection connection = QDBusConnection::sessionBus();
  if (!connection.isConnected()) {
    return false;
  }

  new OpenSCADAdaptor(this);
  connection.registerObject("/org/openscad/OpenSCAD/Application", this);
  connection.registerService("org.openscad.OpenSCAD");
  new org::openscad::OpenSCAD(QString(), QString(), connection, this);

  name = "DBusInputDriver (" + connection.baseService().toStdString() + ")";

  is_open = true;
  return true;
}

void DBusInputDriver::close()
{

}

void DBusInputDriver::zoom(double zoom) const
{
  InputDriverManager::instance()->sendEvent(new InputEventZoom(zoom, true, false));
}

void DBusInputDriver::zoomTo(double zoom) const
{
  InputDriverManager::instance()->sendEvent(new InputEventZoom(zoom, false, false));
}

void DBusInputDriver::rotate(double x, double y, double z) const
{
  InputDriverManager::instance()->sendEvent(new InputEventRotate(x, y, z, true, false));
}

void DBusInputDriver::rotateTo(double x, double y, double z) const
{
  InputDriverManager::instance()->sendEvent(new InputEventRotate(x, y, z, false, false));
}

void DBusInputDriver::rotateByVector(double x, double y, double z) const
{
  InputDriverManager::instance()->sendEvent(new InputEventRotate2(x, y, z, false));
}

void DBusInputDriver::translate(double x, double y, double z) const
{
  InputDriverManager::instance()->sendEvent(new InputEventTranslate(x, y, z, true, false, false));
}

void DBusInputDriver::translateTo(double x, double y, double z) const
{
  InputDriverManager::instance()->sendEvent(new InputEventTranslate(x, y, z, false, false, false));
}

void DBusInputDriver::action(const QString& name) const
{
  InputDriverManager::instance()->sendEvent(new InputEventAction(name.toStdString(), false));
}

const QStringList DBusInputDriver::getActions() const
{
  QStringList ret;
  for (const auto& action : InputDriverManager::instance()->getActions()) {
    ret << action.name;
  }
  return ret;
}

void DBusInputDriver::buttonPress(uint idx) const
{
  InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(idx, true, false));
  InputDriverManager::instance()->sendEvent(new InputEventButtonChanged(idx, false, false));
}

const QList<double> DBusInputDriver::getRotation() const
{
  return InputDriverManager::instance()->getRotation();
}

const QList<double> DBusInputDriver::getTranslation() const
{
  return InputDriverManager::instance()->getTranslation();
}

const std::string& DBusInputDriver::get_name() const
{
  return this->name;
}

std::string DBusInputDriver::get_info() const
{
  return STR(get_name(), " ", (isOpen() ? "open" : "not open"));
}
