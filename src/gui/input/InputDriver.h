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

#include <QThread>
#include <cstddef>
#include <string>

class InputDriver : public QThread
{
public:
  // Note that those 2 values also relate to the currently
  // static list of fields in the preferences GUI, so updating
  // here needs a change in the UI definition!
  const static size_t max_axis = 9;
  const static size_t max_buttons = 24;

public:
  InputDriver() = default;

  virtual const std::string& get_name() const = 0;
  virtual std::string get_info() const = 0;

  virtual bool open() = 0;
  virtual void close() = 0;

  /*
   * Return if the driver is currently opened. The default implementation
   * simply returns the {@link #isRunning()} status of the thread.
   */
  virtual bool isOpen() const;

  /*
   * Drivers that are not connected to a device and can be opened on
   * application start. No attempt to re-open is made.
   */
  virtual bool openOnce() const;

  virtual size_t getButtonCount() const {return 0;}
  virtual size_t getAxisCount() const {return 0;}
};
