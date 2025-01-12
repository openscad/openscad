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

#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QStringList>
#include <QTemporaryFile>

#include "Settings.h"
#include "gui/OctoPrint.h"
#include "io/export.h"
#include "geometry/Geometry.h"

bool ExternalToolInterface::exportTemporaryFile(const std::shared_ptr<const Geometry>& rootGeometry, 
  const QString& sourceFileName, const Camera *const camera)
{
  const ExportInfo exportInfo = {
      .format = exportFormat_,
      .title = sourceFileName.toStdString(),
      .sourceFilePath = sourceFileName.toStdString(),
      .camera = camera,
      .defaultColor = { 0xf9, 0xd7, 0x2c, 255 } // Cornfield: CGAL_FACE_FRONT_COLOR
  };

  // FIXME: Remove original suffix first
  QTemporaryFile exportFile{getTempDir().filePath(
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

  sourceFilename_ = sourceFileName.toStdString();
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

QDir LocalProgramService::getTempDir() const
{
  const auto& tempDirConfig = Settings::Settings::localAppTempDir.value();
  if (tempDirConfig.empty()) {
    return QDir::temp();
  }
  const auto tempDir = QDir{QString::fromStdString(tempDirConfig)};
  if (!tempDir.exists()) {
    LOG(message_group::Warning, "Configured temporary directory does not exist: '%1$s'", tempDirConfig);
    return QDir::temp();
  }
  return tempDir;
}

bool LocalProgramService::process(const std::string& displayName, std::function<bool (double)> progress_cb) 
{
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);

  const QString application = QString::fromStdString(Settings::Settings::localAppExecutable.value());

  if (application.trimmed().isEmpty()) {
    LOG(message_group::Error, "No application configured, check Preferences -> 3D Print -> Local Application");
    return false;
  }

  QStringList args;
  const auto info = QFileInfo(QString::fromStdString(exportedFilename_));
  for (const auto& arg : Settings::Settings::localAppParameterList.items()) {
    switch (arg.type) {
    case Settings::LocalAppParameterType::string:
      args.append(QString::fromStdString(arg.value));
      break;
    case Settings::LocalAppParameterType::file:
      args.append(QString::fromStdString(exportedFilename_));
      break;
    case Settings::LocalAppParameterType::dir:
      args.append(info.absoluteDir().path());
      break;
    case Settings::LocalAppParameterType::extension:
      args.append(info.suffix());
      break;
    case Settings::LocalAppParameterType::source:
      args.append(QString::fromStdString(sourceFilename_));
      break;
    case Settings::LocalAppParameterType::sourcedir:
      args.append(QFileInfo(QString::fromStdString(sourceFilename_)).absoluteDir().path());
      break;
    default:
      break;
    }
  }
  const auto argsStr = args.empty() ? "<none>" : "['" + args.join("', '").toStdString() + "']";
  PRINTDB("Running application '%s' with arguments: %s", application.toStdString() % argsStr);
#ifdef Q_OS_MACOS
  if (!process.startDetached("open", QStringList({"-a", application, "--args"}) + args)) {
#else
  if (!process.startDetached(application, args)) {
#endif
    LOG(message_group::Error, "Could not start Slicer '%1$s': %2$s", application.toStdString(), process.errorString().toStdString());
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
