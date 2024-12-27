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

#include <memory>

#include <QtCore/qstring.h>

#include "gui/PrintService.h"
#include "geometry/Geometry.h"
#include "gui/Settings.h"
#include "io/export.h"

class ExternalToolInterface
{
public:
  ExternalToolInterface(FileFormat fileFormat) : exportFormat_(fileFormat) {}
  virtual ~ExternalToolInterface() = default;
  
  virtual bool exportTemporaryFile(const std::shared_ptr<const Geometry>& rootGeometry, const QString& sourceFileName, const Camera *const camera);
  virtual bool process(const std::string& displayName, std::function<bool (double)>) = 0;

  FileFormat fileFormat() const { return exportFormat_; }
  virtual std::string getURL() const { return ""; };
protected:
  FileFormat exportFormat_;
  std::string exportedFilename_;
};


class ExternalPrintService : public ExternalToolInterface
{
public:
  ExternalPrintService(FileFormat fileFormat, const PrintService *printService) : ExternalToolInterface(fileFormat), printService(printService) {}
  bool process(const std::string& displayName, std::function<bool (double)>) override;
  std::string getURL() const override {return url;}

private:
  std::string url;
  const PrintService *printService;
};

std::unique_ptr<ExternalPrintService> createExternalPrintService(const PrintService *printService, FileFormat fileFormat);

class OctoPrintService : public ExternalToolInterface
{
  public:
  OctoPrintService(FileFormat fileFormat) : ExternalToolInterface(fileFormat) {}
  bool process(const std::string& displayName, std::function<bool (double)>) override;

  private:
  std::string action;
  std::string slicerEngine;
  std::string slicerAction;
};

std::unique_ptr<OctoPrintService> createOctoPrintService(FileFormat fileFormat);

class LocalProgramService : public ExternalToolInterface
{
  public:
  LocalProgramService(FileFormat fileFormat) : ExternalToolInterface(fileFormat) {}
  bool process(const std::string& displayName, std::function<bool (double)>) override;
};

std::unique_ptr<LocalProgramService> createLocalProgramService(FileFormat fileFormat);
