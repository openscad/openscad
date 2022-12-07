/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
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

#include <functional>

#include <QObject>
#include <utility>

using network_progress_func_t = std::function<bool (double)>;

class NetworkSignal : public QObject
{
  Q_OBJECT;

  using callback_t = std::function<void (qint64, qint64)>;

  NetworkSignal(QObject *parent, callback_t callback) : QObject(parent), callback(std::move(callback)) { }

public slots:
  void network_progress(qint64 bytesSent, qint64 bytesTotal) {
    callback(bytesSent, bytesTotal);
  }

private:
  callback_t callback;

  template <typename T> friend class NetworkRequest;
};
