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

#include "gui/ExternalToolInterface.h"

#include <QtCore/qtemporaryfile.h>
#include <QtCore/QDir>

#include "gui/OctoPrint.h"
#include "io/export.h"
#include "geometry/Geometry.h"

bool ExternalToolInterface::exportTemporaryFile(const std::shared_ptr<const Geometry>& rootGeometry, 
  const QString& sourceFileName, const Camera *const camera)
{
  const ExportInfo exportInfo = {
      .format = exportFormat_,
      .sourceFilePath = sourceFileName.toStdString(),
      .camera = camera,
  };

  // FIXME: Remove original suffix first
  QTemporaryFile exportFile{QDir::temp().filePath(
    QString("%1.XXXXXX.%2").
      arg(QString::fromStdString(exportInfo.sourceFilePath)).
      arg(QString::fromStdString(fileformat::toSuffix(exportFormat_))))};
  // FIXME: When is it safe to remove the file?
  // * Octoprint: After uploading?
  // * PrintService: After uploading?
  // * Local slicer: Never?
  exportFile.setAutoRemove(false);
  if (!exportFile.open()) {
    LOG("Could not open temporary file.");
    return false;
  }
  const QString exportFileName = exportFile.fileName();


  exportedFilename_ = exportFileName.toStdString();
  const bool ok = exportFileByName(rootGeometry, exportedFilename_, exportInfo);
  LOG("Exported temporary file %1$s", exportedFilename_);
  return ok;
}

bool OctoPrintService::process(const std::string& displayName, std::function<bool (double)> progress_cb) 
{
  const OctoPrint octoPrint;

  try {
    const QString fileUrl = octoPrint.upload(QString::fromStdString(exportedFilename_), QString::fromStdString(displayName), progress_cb);
    if (this->action == "upload") {
      return true;
    }

    const QString slicer = QString::fromStdString(Settings::Settings::octoPrintSlicerEngine.value());
    const QString profile = QString::fromStdString(Settings::Settings::octoPrintSlicerProfile.value());
    octoPrint.slice(fileUrl, slicer, profile, action != "slice", action == "print");
  } catch (const NetworkException& e) {
    LOG(message_group::Error, "%1$s", e.getErrorMessage());
  }
  return true;
}

bool LocalProgramService::process(const std::string& displayName, std::function<bool (double)> progress_cb) 
{
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);

  const QString slicer = QString::fromStdString(Settings::Settings::localSlicerExecutable.value());
#ifdef Q_OS_MACOS
  if(!process.startDetached("open", {"-a", slicer, QString::fromStdString(exportedFilename_)})) {
#else	  
  if(!process.startDetached(slicer, {QString::fromStdString(exportedFilename_)})) {
#endif
    LOG(message_group::Error, "Could not start Slicer '%1$s': %2$s", slicer.toStdString(), process.errorString().toStdString());
    const auto output = process.readAll();
    if (output.length() > 0) {
      LOG(message_group::Error, "Output: %1$s", output.toStdString());
    }
  }
  return true;
}

bool ExternalPrintService::process(const std::string& displayName, std::function<bool (double)> progress_cb) 
{
  QFile file(QString::fromStdString(exportedFilename_));
  if (!file.open(QIODevice::ReadOnly)) {
    LOG(message_group::Error, "Unable to open exported STL file.");
    return false;
  }
  const QString fileContentBase64 = file.readAll().toBase64();

  if (fileContentBase64.length() > printService->getFileSizeLimit()) {
    const auto msg = QString{_("Exported design exceeds the service upload limit of (%1 MB).")}.arg(printService->getFileSizeLimitMB());
    // FIXME: Move back to MainWindow
//    QMessageBox::warning(this, _("Upload Error"), msg, QMessageBox::Ok);
    //LOG(message_group::Error, "%1$s", msg.toStdString());
    return false;
  }
  try {
    const QString partUrl = printService->upload(QString::fromStdString(displayName), fileContentBase64, progress_cb);
    this->url = partUrl.toStdString() ;
  } catch (const NetworkException& e) {
    LOG(message_group::Error, "%1$s", e.getErrorMessage());
  }
  return true;
}

std::unique_ptr<ExternalPrintService> createExternalPrintService(const PrintService *printService, FileFormat fileFormat) {
  return std::make_unique<ExternalPrintService>(fileFormat, printService);
}

std::unique_ptr<OctoPrintService> createOctoPrintService(FileFormat fileFormat)
{
  auto octoPrintService = std::make_unique<OctoPrintService>(fileFormat);


// TODO: set action, slicerEngine, slicerAction
//    const std::string& action = Settings::Settings::octoPrintAction.value();
    // const QString slicer = QString::fromStdString(Settings::Settings::octoPrintSlicerEngine.value());
    // const QString profile = QString::fromStdString(Settings::Settings::octoPrintSlicerProfile.value());


  return octoPrintService;
}

std::unique_ptr<LocalProgramService> createLocalProgramService(FileFormat fileFormat) {
  return std::make_unique<LocalProgramService>(fileFormat);
}
