/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
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

#include "openscad_gui.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QIcon>

#include "core/parsersettings.h"
#include "FontCache.h"
#include "geometry/Geometry.h"
#include "gui/AppleEvents.h"
#include "gui/LaunchingScreen.h"
#include "gui/MainWindow.h"
#include "gui/OpenSCADApp.h"
#include "gui/QSettingsCached.h"
#include "gui/Settings.h"
#include "openscad.h"
#include "utils/printutils.h"

#include "gui/input/InputDriverManager.h"
#ifdef ENABLE_HIDAPI
#include "gui/input/HidApiInputDriver.h"
#endif
#ifdef ENABLE_SPNAV
#include "gui/input/SpaceNavInputDriver.h"
#endif
#ifdef ENABLE_JOYSTICK
#include "gui/input/JoystickInputDriver.h"
#endif
#ifdef ENABLE_DBUS
#include "gui/input/DBusInputDriver.h"
#endif
#ifdef ENABLE_QGAMEPAD
#include "gui/input/QGamepadInputDriver.h"
#endif

Q_DECLARE_METATYPE(Message);
Q_DECLARE_METATYPE(std::shared_ptr<const Geometry>);

extern std::string arg_colorscheme;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace {

// Only if "fileName" is not absolute, prepend the "absoluteBase".
QString assemblePath(const std::filesystem::path& absoluteBaseDir,
                     const std::string& fileName) {
  if (fileName.empty()) return "";
  auto qsDir = QString::fromLocal8Bit(absoluteBaseDir.generic_string().c_str());
  auto qsFile = QString::fromLocal8Bit(fileName.c_str());
  // if qsfile is absolute, dir is ignored. (see documentation of QFileInfo)
  QFileInfo fileInfo(qsDir, qsFile);
  return fileInfo.absoluteFilePath();
}

}  // namespace

void dialogThreadFunc(FontCacheInitializer *initializer)
{
  initializer->run();
}

void dialogInitHandler(FontCacheInitializer *initializer, void *)
{
  QFutureWatcher<void> futureWatcher;
  QObject::connect(&futureWatcher, SIGNAL(finished()), scadApp, SLOT(hideFontCacheDialog()));

  auto future = QtConcurrent::run([initializer] {
    return dialogThreadFunc(initializer);
  });
  futureWatcher.setFuture(future);

  // We don't always get the started() signal, so we start manually
  QMetaObject::invokeMethod(scadApp, "showFontCacheDialog");

  // Block, in case we're in a separate thread, or the dialog was closed by the user
  futureWatcher.waitForFinished();

  // We don't always receive the finished signal. We still need the signal to break
  // out of the exec() though.
  QMetaObject::invokeMethod(scadApp, "hideFontCacheDialog");
}

#ifdef Q_OS_WIN
void registerDefaultIcon(QString applicationFilePath) {
  // Not using cached instance here, so this needs to be in a
  // separate scope to ensure the QSettings instance is released
  // directly after use.
  QSettings reg_setting(QLatin1String("HKEY_CURRENT_USER"), QSettings::NativeFormat);
  auto appPath = QDir::toNativeSeparators(applicationFilePath + QLatin1String(",1"));
  reg_setting.setValue(QLatin1String("Software/Classes/OpenSCAD_File/DefaultIcon/Default"), QVariant(appPath));
}
#else
void registerDefaultIcon(const QString&) { }
#endif

#ifdef OPENSCAD_SUFFIX
#define DESKTOP_FILENAME "openscad" OPENSCAD_SUFFIX
#else
#define DESKTOP_FILENAME "openscad"
#endif

int gui(std::vector<std::string>& inputFiles, const std::filesystem::path& original_path, int argc, char **argv)
{
  OpenSCADApp app(argc, argv);
  // remove ugly frames in the QStatusBar when using additional widgets
  app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

  // set up groups for QSettings
  QCoreApplication::setOrganizationName("OpenSCAD");
  QCoreApplication::setOrganizationDomain("openscad.org");
  QCoreApplication::setApplicationName("OpenSCAD");
  QCoreApplication::setApplicationVersion(TOSTRING(OPENSCAD_VERSION));
  QGuiApplication::setApplicationDisplayName("OpenSCAD");
  QGuiApplication::setDesktopFileName(DESKTOP_FILENAME);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#ifdef Q_OS_MACOS
  app.setWindowIcon(QIcon(":/icon-macos.png"));
#else
  app.setWindowIcon(QIcon(":/logo.png"));
#endif

  // Other global settings
  qRegisterMetaType<Message>();
  qRegisterMetaType<std::shared_ptr<const Geometry>>();

  FontCache::registerProgressHandler(dialogInitHandler);

  parser_init();

  QSettingsCached settings;
  if (settings.value("advanced/localization", true).toBool()) {
    localization_init();
  }

#ifdef Q_OS_MACOS
  installAppleEventHandlers();
#endif

  registerDefaultIcon(app.applicationFilePath());

#ifdef OPENSCAD_UPDATER
  AutoUpdater *updater = new SparkleAutoUpdater;
  AutoUpdater::setUpdater(updater);
  if (updater->automaticallyChecksForUpdates()) updater->checkForUpdates();
  updater->init();
#endif

  set_render_color_scheme(arg_colorscheme, false);
  auto noInputFiles = false;

  if (!inputFiles.size()) {
    noInputFiles = true;
    inputFiles.emplace_back("");
  }

  auto showOnStartup = settings.value("launcher/showOnStartup");
  if (noInputFiles && (showOnStartup.isNull() || showOnStartup.toBool())) {
    auto launcher = new LaunchingScreen();
    auto dialogResult = launcher->exec();
    if (dialogResult == QDialog::Accepted) {
      if (launcher->isForceShowEditor()) {
        settings.setValue("view/hideEditor", false);
      }
      auto files = launcher->selectedFiles();
      // If nothing is selected in the launching screen, leave
      // the "" dummy in inputFiles to open an empty MainWindow.
      if (!files.empty()) {
        inputFiles.clear();
        for (const auto& f : files) {
          inputFiles.push_back(f.toStdString());
        }
      }
      delete launcher;
    } else {
      return 0;
    }
  }

  QStringList inputFilesList;
  for (const auto& infile: inputFiles) {
    inputFilesList.append(assemblePath(original_path, infile));
  }
  new MainWindow(inputFilesList);
  app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(releaseQSettingsCached()));
  app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

#ifdef ENABLE_HIDAPI
  if (Settings::Settings::inputEnableDriverHIDAPI.value()) {
    auto hidApi = new HidApiInputDriver();
    InputDriverManager::instance()->registerDriver(hidApi);
  }
#endif
#ifdef ENABLE_SPNAV
  if (Settings::Settings::inputEnableDriverSPNAV.value()) {
    auto spaceNavDriver = new SpaceNavInputDriver();
    bool spaceNavDominantAxisOnly = Settings::Settings::inputEnableDriverHIDAPI.value();
    spaceNavDriver->setDominantAxisOnly(spaceNavDominantAxisOnly);
    InputDriverManager::instance()->registerDriver(spaceNavDriver);
  }
#endif
#ifdef ENABLE_JOYSTICK
  if (Settings::Settings::inputEnableDriverJOYSTICK.value()) {
    std::string nr = STR(Settings::Settings::joystickNr.value());
    auto joyDriver = new JoystickInputDriver();
    joyDriver->setJoystickNr(nr);
    InputDriverManager::instance()->registerDriver(joyDriver);
  }
#endif
#ifdef ENABLE_QGAMEPAD
  if (Settings::Settings::inputEnableDriverQGAMEPAD.value()) {
    auto qGamepadDriver = new QGamepadInputDriver();
    InputDriverManager::instance()->registerDriver(qGamepadDriver);
  }
#endif
#ifdef ENABLE_DBUS
  if (Feature::ExperimentalInputDriverDBus.is_enabled()) {
    if (Settings::Settings::inputEnableDriverDBUS.value()) {
      auto dBusDriver = new DBusInputDriver();
      InputDriverManager::instance()->registerDriver(dBusDriver);
    }
  }
#endif

  InputDriverManager::instance()->init();
  int rc = app.exec();
  const auto& windows = scadApp->windowManager.getWindows();
  while (!windows.empty()) delete *windows.begin();
  return rc;
}
