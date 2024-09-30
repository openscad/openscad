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

#include <mutex>

#include <QString>
#include <QJsonDocument>

#include "gui/Network.h"

class PrintService
{
public:
  static PrintService *inst();

  bool isEnabled() const { return enabled; }
  const QString getService() const { return service; }
  const QString getDisplayName() const { return displayName; }
  const QString getApiUrl() const { return apiUrl; }
  long getFileSizeLimit() const { return fileSizeLimitMB * 1024ul * 1024ul; }
  long getFileSizeLimitMB() const { return fileSizeLimitMB; }
  const QString getInfoHtml() const { return infoHtml; }
  const QString getInfoUrl() const { return infoUrl; }

  const QString upload(const QString& exportFileName, const QString& fileName, const network_progress_func_t& progress_func);

private:
  PrintService();
  virtual ~PrintService() = default;

  void init();
  void initService(const QJsonObject& serviceObject);

  bool enabled;
  QString service;
  QString displayName;
  QString apiUrl;
  int fileSizeLimitMB;
  QString infoUrl;
  QString infoHtml;

  static std::mutex printServiceMutex;
};