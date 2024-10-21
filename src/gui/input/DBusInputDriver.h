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

#include <QList>
#include <QString>
#include <QStringList>
#include "gui/input/InputDriver.h"
#include <string>

class DBusInputDriver : public InputDriver
{
  Q_OBJECT

  bool is_open{false};

  std::string name;

public:
  DBusInputDriver();
  void run() override;
  bool open() override;
  void close() override;
  bool isOpen() const override;
  bool openOnce() const override;

  const std::string& get_name() const override;
  std::string get_info() const override;

public slots:
  void zoom(double zoom) const;
  void zoomTo(double zoom) const;
  void rotate(double x, double y, double z) const;
  void rotateTo(double x, double y, double z) const;
  void rotateByVector(double x, double y, double z) const;
  void translate(double x, double y, double z) const;
  void translateTo(double x, double y, double z) const;
  void action(const QString& action) const;
  void buttonPress(uint idx) const;
  const QList<double> getRotation() const;
  const QList<double> getTranslation() const;
  const QStringList getActions() const;
};
