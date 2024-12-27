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
#include <QObject>
#include <QString>
#include <list>
#include <QWidget>
#include <QThread>
#include <QTimer>
#include <QIcon>

#include <cstddef>
#include <string>

#include "gui/input/InputDriver.h"
#include "gui/input/InputEventMapper.h"

class MainWindow;

struct ActionStruct {
  QString name;
  QString description;
  QIcon icon;
};

class InputDriverManager : public QObject
{
  Q_OBJECT
private:
  using drivers_t = std::list<InputDriver *>;

  drivers_t drivers;

  std::list<ActionStruct> actions;

  InputEventMapper mapper;

  MainWindow *currentWindow{nullptr};

  QTimer *timer{nullptr};

  static InputDriverManager *self;

  void postEvent(InputEvent *event);

public:
  InputDriverManager() = default;

  void sendEvent(InputEvent *event);

  void init();
  std::string listDrivers() const;
  std::string listDriverInfos() const;
  void registerDriver(InputDriver *driver);
  void unregisterDriver(InputDriver *driver);
  void closeDrivers();
  void registerActions(const QList<QAction *>& actions, const QString& parent = QString(""), const QString& target = QString(""));

  static InputDriverManager *instance();

  const std::list<ActionStruct>& getActions() const;
  QList<double> getTranslation() const;
  QList<double> getRotation() const;

  size_t getButtonCount() const;
  size_t getAxisCount() const;

public slots:
  void onInputMappingUpdated();
  void onInputCalibrationUpdated();
  void onInputGainUpdated();

private slots:
  void onTimeout();
  void doOpen(bool firstOpen);
  void onFocusChanged(QWidget *, QWidget *);

  friend class InputEventMapper;
};
