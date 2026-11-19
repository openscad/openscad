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
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <functional>
#include <memory>
#include <string>

#include "core/Settings.h"
#include "geometry/Geometry.h"
#include "glview/Camera.h"
#include "gui/OctoPrint.h"
#include "io/export.h"
#include "utils/printutils.h"

namespace {

QString getArgValue(const Settings::LocalAppParameter& arg, const std::string& exportedFilename,
                    const std::string& sourceFilename)
{
  const QFileInfo info(QString::fromStdString(exportedFilename));
  switch (arg.type) {
  case Settings::LocalAppParameterType::string:    return QString::fromStdString(arg.value);
  case Settings::LocalAppParameterType::file:      return QString::fromStdString(exportedFilename);
  case Settings::LocalAppParameterType::dir:       return info.absoluteDir().path();
  case Settings::LocalAppParameterType::extension: return info.suffix();
  case Settings::LocalAppParameterType::source:    return QString::fromStdString(sourceFilename);
  case Settings::LocalAppParameterType::sourcedir:
    return QFileInfo(QString::fromStdString(sourceFilename)).absoluteDir().path();
  default: return {};
  }
}

}  // namespace

bool ExternalToolInterface::exportTemporaryFile(const std::shared_ptr<const Geometry>& rootGeometry,
                                                const QString& sourceFileName,
                                                const Camera *const camera)
{
  // FIXME: Remove original suffix first
  QTemporaryFile exportFile{
    getTempDir().filePath(QString("%1.XXXXXX.%2")
                            .arg(QString::fromStdString(sourceFileName.toStdString()))
                            .arg(QString::fromStdString(fileformat::toSuffix(exportFormat_))))};
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
  ExportInfo exportInfo = createExportInfo(exportFormat_, fileformat::info(exportFormat_),
                                           sourceFileName.toStdString(), camera, {});
  const bool ok = exportFileByName(rootGeometry, exportedFilename_, exportInfo);
  LOG("Exported temporary file %1$s", exportedFilename_);
  return ok;
}

bool OctoPrintService::process(const std::string& displayName, std::function<bool(double)> progress_cb)
{
  const OctoPrint octoPrint;

  try {
    const QString fileUrl = octoPrint.upload(QString::fromStdString(exportedFilename_),
                                             QString::fromStdString(displayName), progress_cb);
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

bool LocalProgramService::process(const std::string& displayName,
                                  std::function<bool(double)> progress_cb)
{
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);

  const QString application = QString::fromStdString(Settings::Settings::localAppExecutable.value());

  if (application.trimmed().isEmpty()) {
    LOG(message_group::Error,
        "No application configured, check Preferences -> 3D Print -> Local Application");
    return false;
  }

#ifdef Q_OS_MACOS
  QStringList fileArgs, otherArgs;
  for (const auto& arg : Settings::Settings::localAppParameterList.value()) {
    if (arg.type == Settings::LocalAppParameterType::file) {
      fileArgs << getArgValue(arg, exportedFilename_, sourceFilename_);
    } else {
      otherArgs << getArgValue(arg, exportedFilename_, sourceFilename_);
    }
  }

  QStringList commandArgs(fileArgs);
  commandArgs << "-a" << application;
  if (!otherArgs.isEmpty()) {
    commandArgs << "--args" << otherArgs;
  }
  PRINTD("Executing: open " + commandArgs.join(" ").toStdString());
  if (!process.startDetached("open", commandArgs)) {
#else
  QStringList args;
  for (const auto& arg : Settings::Settings::localAppParameterList.value()) {
    args << getArgValue(arg, exportedFilename_, sourceFilename_);
  }
  PRINTD("Executing: " + application.toStdString() + " " + args.join(" ").toStdString());
  if (!process.startDetached(application, args)) {
#endif
    LOG(message_group::Error, "Could not start local application '%1$s': %2$s",
        application.toStdString(), process.errorString().toStdString());
    const auto output = process.readAll();
    if (output.length() > 0) {
      LOG(message_group::Error, "Output: %1$s", output.toStdString());
    }
  }
  return true;
}

bool ExternalPrintService::process(const std::string& displayName,
                                   std::function<bool(double)> progress_cb)
{
  QFile file(QString::fromStdString(exportedFilename_));
  if (!file.open(QIODevice::ReadOnly)) {
    LOG(message_group::Error, "Unable to open exported STL file.");
    return false;
  }
  const QString fileContentBase64 = file.readAll().toBase64();

  if (fileContentBase64.length() > printService->getFileSizeLimit()) {
    const auto msg = QString{_("Exported design exceeds the service upload limit of (%1 MB).")}.arg(
      printService->getFileSizeLimitMB());
    // FIXME: Move back to MainWindow
    //    QMessageBox::warning(this, _("Upload Error"), msg, QMessageBox::Ok);
    // LOG(message_group::Error, "%1$s", msg.toStdString());
    return false;
  }
  try {
    const QString partUrl =
      printService->upload(QString::fromStdString(displayName), fileContentBase64, progress_cb);
    this->url = partUrl.toStdString();
  } catch (const NetworkException& e) {
    LOG(message_group::Error, "%1$s", e.getErrorMessage());
  }
  return true;
}

std::unique_ptr<ExternalPrintService> createExternalPrintService(const PrintService *printService,
                                                                 FileFormat fileFormat)
{
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

std::unique_ptr<LocalProgramService> createLocalProgramService(FileFormat fileFormat)
{
  return std::make_unique<LocalProgramService>(fileFormat);
}

namespace {

constexpr auto PRUSASLICER_FLATPAK_ID = "com.prusa3d.PrusaSlicer";
constexpr auto PRUSASLICER_SNAP_NAME = "prusa-slicer";

bool isExecutableFile(const QString& path)
{
  const QFileInfo info(path);
  return info.exists() && info.isFile() && info.isExecutable();
}

// Snap binaries live in /snap/bin and are wrappers around /usr/bin/snap.
bool isSnapWrapper(const QString& path)
{
  return QFileInfo(path).absoluteFilePath().startsWith("/snap/");
}

PrusaSlicerService::Installation classifyExplicit(const QString& configured)
{
  using Kind = PrusaSlicerService::InstallKind;
  if (configured.endsWith(".app") || configured.endsWith(".app/")) {
    return {Kind::MacApp, configured};
  }
  if (configured == PRUSASLICER_FLATPAK_ID) {
    const auto flatpak = QStandardPaths::findExecutable("flatpak");
    if (!flatpak.isEmpty()) return {Kind::Flatpak, flatpak};
  }
  QString path = configured;
  if (!QFileInfo(path).isAbsolute()) {
    const auto found = QStandardPaths::findExecutable(path);
    if (!found.isEmpty()) path = found;
  }
  return {isSnapWrapper(path) ? Kind::Snap : Kind::Executable, path};
}

}  // namespace

QString PrusaSlicerService::Installation::describe() const
{
  switch (kind) {
  case InstallKind::Executable: return path;
  case InstallKind::Flatpak:    return QString("Flatpak %1").arg(PRUSASLICER_FLATPAK_ID);
  case InstallKind::Snap:       return QString("Snap %1").arg(path);
  case InstallKind::MacApp:     return path;
  default:                      return "not found";
  }
}

PrusaSlicerService::Installation PrusaSlicerService::locate()
{
  const QString configured =
    QString::fromStdString(Settings::Settings::prusaSlicerExecutable.value()).trimmed();
  if (!configured.isEmpty()) {
    return classifyExplicit(configured);
  }

  const QString home = QDir::homePath();

#if defined(Q_OS_MACOS)
  for (const auto& app :
       {QString("/Applications/PrusaSlicer.app"), home + "/Applications/PrusaSlicer.app",
        QString("/Applications/Original Prusa Drivers/PrusaSlicer.app")}) {
    if (QFileInfo(app).isDir()) return {InstallKind::MacApp, app};
  }
#elif defined(Q_OS_WIN)
  for (const auto& name : {"prusa-slicer.exe", "prusa-slicer-console.exe"}) {
    const auto found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) return {InstallKind::Executable, found};
  }
  QStringList roots;
  for (const auto& var : {"ProgramW6432", "ProgramFiles", "ProgramFiles(x86)"}) {
    const auto v = qEnvironmentVariable(var);
    if (!v.isEmpty() && !roots.contains(v)) roots << v;
  }
  for (const auto& root : roots) {
    for (const auto& sub : {"Prusa3D/PrusaSlicer", "PrusaSlicer"}) {
      const auto exe = QDir(root).filePath(QString(sub) + "/prusa-slicer.exe");
      if (QFileInfo::exists(exe)) return {InstallKind::Executable, exe};
    }
  }
#else
  // Native install on PATH (distro package, extracted tarball, AppImage symlink).
  QString snapCandidate;
  for (const auto& name : {"prusa-slicer", "PrusaSlicer", "prusaslicer"}) {
    const auto found = QStandardPaths::findExecutable(name);
    if (found.isEmpty()) continue;
    if (isSnapWrapper(found)) {
      if (snapCandidate.isEmpty()) snapCandidate = found;
      continue;
    }
    return {InstallKind::Executable, found};
  }

  // Flatpak (user or system install). Preferred over Snap: it is what Prusa publishes.
  const auto flatpak = QStandardPaths::findExecutable("flatpak");
  if (!flatpak.isEmpty()) {
    const QStringList flatpakRoots = {home + "/.local/share/flatpak/app", "/var/lib/flatpak/app"};
    for (const auto& root : flatpakRoots) {
      if (QFileInfo(QDir(root).filePath(PRUSASLICER_FLATPAK_ID)).isDir()) {
        return {InstallKind::Flatpak, flatpak};
      }
    }
  }

  // AppImage dropped into a usual place; pick the newest by name.
  for (const auto& dir : {home + "/Applications", home + "/.local/bin", home + "/bin",
                          home + "/Downloads", home + "/opt", QString("/opt")}) {
    auto images =
      QDir(dir).entryList({"PrusaSlicer*.AppImage", "prusa-slicer*.AppImage", "PrusaSlicer*.appimage"},
                          QDir::Files, QDir::Name | QDir::Reversed);
    for (const auto& image : images) {
      const auto path = QDir(dir).filePath(image);
      if (isExecutableFile(path)) return {InstallKind::Executable, path};
    }
  }

  if (!snapCandidate.isEmpty()) return {InstallKind::Snap, snapCandidate};
#endif
  return {};
}

PrusaSlicerService::PrusaSlicerService(FileFormat fileFormat)
  : ExternalToolInterface(fileFormat), installation_(locate())
{
}

QDir PrusaSlicerService::getTempDir() const
{
  // Sandboxed builds cannot see the host /tmp, so drop the file where the sandbox can read it.
  QString dir;
  switch (installation_.kind) {
  case InstallKind::Flatpak:
    dir = QDir::homePath() + "/.var/app/" + PRUSASLICER_FLATPAK_ID + "/cache/openscad";
    break;
  case InstallKind::Snap:
    dir = QDir::homePath() + "/snap/" + PRUSASLICER_SNAP_NAME + "/common/openscad";
    break;
  default: {
    const auto& tempDirConfig = Settings::Settings::localAppTempDir.value();
    if (!tempDirConfig.empty() && QDir(QString::fromStdString(tempDirConfig)).exists()) {
      return QDir{QString::fromStdString(tempDirConfig)};
    }
    return QDir::temp();
  }
  }
  if (!QDir().mkpath(dir)) {
    LOG(message_group::Warning, "Could not create directory '%1$s', using system temp dir",
        dir.toStdString());
    return QDir::temp();
  }
  return QDir{dir};
}

bool PrusaSlicerService::process(const std::string& /*displayName*/,
                                 std::function<bool(double)> /*progress_cb*/)
{
  if (installation_.kind == InstallKind::NotFound) {
    LOG(message_group::Error,
        "PrusaSlicer not found. Install it, or set its path in Preferences -> 3D Print -> "
        "PrusaSlicer.");
    return false;
  }

  const QString file = QString::fromStdString(exportedFilename_);
  QString program;
  QStringList args;
  switch (installation_.kind) {
  case InstallKind::MacApp:
    // `open` hands the file to an already running instance, or starts one.
    program = "open";
    args << "-a" << installation_.path << file;
    break;
  case InstallKind::Flatpak:
    program = installation_.path;
    args << "run" << QString("--filesystem=%1:ro").arg(QFileInfo(file).absolutePath())
         << PRUSASLICER_FLATPAK_ID << "--single-instance" << file;
    break;
  case InstallKind::Snap:
  case InstallKind::Executable:
  default:
    program = installation_.path;
    // Reuse a running PrusaSlicer window instead of spawning a new one each time.
    args << "--single-instance" << file;
    break;
  }

  LOG("Opening in PrusaSlicer (%1$s): %2$s", installation_.describe().toStdString(), file.toStdString());
  PRINTD("Executing: " + program.toStdString() + " " + args.join(" ").toStdString());
  if (!QProcess::startDetached(program, args)) {
    LOG(message_group::Error, "Could not start PrusaSlicer '%1$s'", program.toStdString());
    return false;
  }
  return true;
}

std::unique_ptr<PrusaSlicerService> createPrusaSlicerService(FileFormat fileFormat)
{
  return std::make_unique<PrusaSlicerService>(fileFormat);
}
