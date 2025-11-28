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
#include <QtCore/qstringliteral.h>
#include <memory>
#include <filesystem>
#include <string>
#include <vector>

#include <QtGlobal>
#include <Qt>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QIcon>
#include <QObject>
#include <QPalette>
#include <QStyleHints>
#include <QStringList>
#include <QtConcurrentRun>

#include "Feature.h"
#include "core/parsersettings.h"
#include "core/Settings.h"
#include "FontCache.h"
#include "geometry/Geometry.h"
#include "gui/AppleEvents.h"
#include "gui/input/InputDriverManager.h"
#include "version.h"
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
#include "gui/LaunchingScreen.h"
#include "gui/MainWindow.h"
#include "gui/OpenSCADApp.h"
#include "gui/QSettingsCached.h"
#include "gui/Preferences.h"
#include "openscad.h"
#include "platform/CocoaUtils.h"
#include "utils/printutils.h"

#ifdef ENABLE_GUI_TESTS
#include "guitests/guitests.h"
#endif

Q_DECLARE_METATYPE(Message);
Q_DECLARE_METATYPE(std::shared_ptr<const Geometry>);

extern std::string arg_colorscheme;

namespace {

// Check if running with light or dark theme. This should really just be used
// to switch the icon theme globally.
//
// For applying a color change, e.g. highlighting the background of an input
// field, see:
// UIUtils::blendForBackgroundColorStyleSheet(const QColor& input, const QColor& blend)

bool isDarkMode()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  const auto scheme = QGuiApplication::styleHints()->colorScheme();
  return scheme == Qt::ColorScheme::Dark;
#else
  const QPalette defaultPalette;
  const auto& text = defaultPalette.color(QPalette::WindowText);
  const auto& window = defaultPalette.color(QPalette::Window);
  return text.lightness() > window.lightness();
#endif  // QT_VERSION
}

}  // namespace

namespace {

// Only if "fileName" is not absolute, prepend the "absoluteBase".
QString assemblePath(const std::filesystem::path& absoluteBaseDir, const std::string& fileName)
{
  if (fileName.empty()) return "";
  auto qsDir = QString::fromStdString(absoluteBaseDir.generic_string());
  auto qsFile = QString::fromStdString(fileName);
  // if qsfile is absolute, dir is ignored. (see documentation of QFileInfo)
  const QFileInfo fileInfo(qsDir, qsFile);
  return fileInfo.absoluteFilePath();
}

void dialogThreadFunc(FontCacheInitializer *initializer) { initializer->run(); }

void dialogInitHandler(FontCacheInitializer *initializer, void *)
{
  QFutureWatcher<void> futureWatcher;
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, scadApp,
                   &OpenSCADApp::hideFontCacheDialog);

  auto future = QtConcurrent::run([initializer] { return dialogThreadFunc(initializer); });
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
void registerDefaultIcon(QString applicationFilePath)
{
  // Not using cached instance here, so this needs to be in a
  // separate scope to ensure the QSettings instance is released
  // directly after use.
  QSettings reg_setting(QLatin1String("HKEY_CURRENT_USER"), QSettings::NativeFormat);
  auto appPath = QDir::toNativeSeparators(applicationFilePath + QLatin1String(",1"));
  reg_setting.setValue(QLatin1String("Software/Classes/OpenSCAD_File/DefaultIcon/Default"),
                       QVariant(appPath));
}
#else
void registerDefaultIcon(const QString&) {}
#endif

}  // namespace

#ifdef OPENSCAD_SUFFIX
#define DESKTOP_FILENAME "openscad" OPENSCAD_SUFFIX
#else
#define DESKTOP_FILENAME "openscad"
#endif

int gui(std::vector<std::string>& inputFiles, const std::filesystem::path& original_path, int argc,
        char **argv, const std::string& gui_test, const bool reset_window_settings)
{
  OpenSCADApp app(argc, argv);
  QIcon::setThemeName(isDarkMode() ? "chokusen-dark" : "chokusen");

  // set up groups for QSettings
  QCoreApplication::setOrganizationName("OpenSCAD");
  QCoreApplication::setOrganizationDomain("openscad.org");
  QCoreApplication::setApplicationName("OpenSCAD");
  QCoreApplication::setApplicationVersion(QString::fromStdString(std::string(openscad_versionnumber)));
  QGuiApplication::setApplicationDisplayName("OpenSCAD");
  QGuiApplication::setDesktopFileName(DESKTOP_FILENAME);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

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
  if (reset_window_settings) {
    const auto keys = std::array<std::string, 20>{
      "editor/fontfamily",
      "editor/fontsize",
      "advanced/applicationFontSize",
      "advanced/applicationFontFamily",
      "advanced/consoleFontFamily",
      "advanced/consoleFontSize",
      "advanced/customizerFontFamily",
      "advanced/customizerFontSize",
      "advanced/undockableWindows",
      "window/state",
      "window/geometry",
      "window/position",
      "window/size",
      "view/hideEditor",
      "view/hideConsole",
      "view/hideErrorLog",
      "view/hideAnimate",
      "view/hideCustomizer",
      "view/hideFontList",
      "view/hideViewportControl",
    };
    for (const auto& key : keys) {
      settings.remove(QString::fromStdString(key));
    }
  }

#ifdef Q_OS_MACOS
  installAppleEventHandlers();
#endif

  registerDefaultIcon(app.applicationFilePath());
  app.setApplicationFont(
    GlobalPreferences::inst()->getValue("advanced/applicationFontFamily").toString(),
    GlobalPreferences::inst()->getValue("advanced/applicationFontSize").toUInt());

#ifdef OPENSCAD_UPDATER
  AutoUpdater *updater = new SparkleAutoUpdater;
  AutoUpdater::setUpdater(updater);
  if (updater->automaticallyChecksForUpdates()) updater->checkForUpdates();
  updater->init();
#endif

  QObject::connect(GlobalPreferences::inst(), &Preferences::applicationFontChanged, &app,
                   &OpenSCADApp::setApplicationFont);

  set_render_color_scheme(arg_colorscheme, false);
  auto noInputFiles = false;

  if (!inputFiles.size()) {
    noInputFiles = true;
    inputFiles.emplace_back("");
  }

  auto showOnStartup = settings.value("launcher/showOnStartup");
  if (noInputFiles && (showOnStartup.isNull() || showOnStartup.toBool())) {
    LaunchingScreen launcher;
    if (launcher.exec() == QDialog::Accepted) {
      if (launcher.isForceShowEditor()) {
        settings.setValue("view/hideEditor", false);
      }
      const QStringList files = launcher.selectedFiles();
      // If nothing is selected in the launching screen, leave
      // the "" dummy in inputFiles to open an empty MainWindow.
      if (!files.empty()) {
        inputFiles.clear();
        for (const auto& f : files) {
          inputFiles.push_back(f.toStdString());
        }
      }
    } else {
      return 0;
    }
  }

  QStringList inputFilesList;
  for (const auto& infile : inputFiles) {
    inputFilesList.append(assemblePath(original_path, infile));
  }
  new MainWindow(inputFilesList);
  QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
    QSettingsCached{}.release();
#ifdef Q_OS_MACOS
    CocoaUtils::endApplication();
#endif
  });

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

#ifdef ENABLE_GUI_TESTS
  // Adds a singleshot timer that will be executed when the application will be started.
  // the timer validates that each mainwindow respects the expected UX behavior.
  if (gui_test != "none") {
    QTimer::singleShot(0, [&]() {
      int failureCount = 0;
      for (auto w : app.windowManager.getWindows()) {
        failureCount += runAllTest(w);
      }
      app.exit(failureCount);
    });
  }
#endif  // ENABLE_GUI_TESTS

  InputDriverManager::instance()->init();
  return app.exec();
}
