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

#include <QAbstractButton>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QApplication>
#include <QGuiApplication>
#include <QHostInfo>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QMessageBox>
#include <QObject>
#include <QPalette>
#include <QSurfaceFormat>
#include <QDesktopServices>
#include <QFile>
#include <QFileDevice>
#include <QSessionManager>
#include <QSaveFile>
#include <QSocketNotifier>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QStyleHints>
#include <QVector>
#include <Qt>
#include <QtConcurrentRun>
#include <QtGlobal>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifdef Q_OS_UNIX
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "Feature.h"
#include "FontCache.h"
#include "core/Settings.h"
#include "core/parsersettings.h"
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
#include "gui/Preferences.h"
#include "gui/QSettingsCached.h"
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
// File-local linkage for helpers in this translation unit (through the closing
// brace before gui()). Review: avoid exporting symbols from openscad_gui.cc.

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

void configureOpenGLContext()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  // OpenSCAD still relies on legacy OpenGL compatibility features and GLSL 1.20
  // shaders, so a GLES context is not currently usable. On Wayland/EGL setups Qt
  // can otherwise pick OpenGL ES by default.
  if (qEnvironmentVariableIsEmpty("QT_OPENGL")) {
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  }

  auto format = QSurfaceFormat::defaultFormat();
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
  if (format.depthBufferSize() < 24) format.setDepthBufferSize(24);
  if (format.stencilBufferSize() < 8) format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);
#endif
}

bool shouldOfferAutosaveRestore(const QString& autosavePath, const QString& sessionPath)
{
  const QFileInfo autosaveInfo(autosavePath);
  if (!autosaveInfo.exists()) return false;

  const QFileInfo sessionInfo(sessionPath);
  if (!sessionInfo.exists()) return true;

  return autosaveInfo.lastModified() > sessionInfo.lastModified();
}

enum class AutosaveRecoverChoice {
  RestoreIntoSession,
  DiscardAutosaveOnly,
  StartFreshSession,
  /// User dismissed or chose Cancel — do not delete autosave or change session files.
  KeepForLater,
};

AutosaveRecoverChoice promptAutosaveRestore(const QString& autosavePath)
{
  while (true) {
    QMessageBox box;
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(_("PythonSCAD"));
    box.setText(_("Recovered session data was found."));
    auto *restoreButton = box.addButton(_("Restore"), QMessageBox::AcceptRole);
    auto *cancelButton = box.addButton(_("Cancel"), QMessageBox::RejectRole);
    auto *discardButton = box.addButton(_("Discard recovery only"), QMessageBox::ApplyRole);
    auto *startFreshButton = box.addButton(_("Start fresh"), QMessageBox::DestructiveRole);
    auto *showButton = box.addButton(_("Show File"), QMessageBox::ActionRole);
    box.setDefaultButton(restoreButton);
    box.exec();

    QAbstractButton *clicked = box.clickedButton();
    if (!clicked || clicked == cancelButton) {
      return AutosaveRecoverChoice::KeepForLater;
    }

    if (clicked == showButton) {
      const QString dirPath = QFileInfo(autosavePath).absolutePath();
      QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
      continue;
    }

    if (clicked == restoreButton) {
      return AutosaveRecoverChoice::RestoreIntoSession;
    }
    if (clicked == startFreshButton) {
      return AutosaveRecoverChoice::StartFreshSession;
    }

    if (clicked == discardButton) {
      return AutosaveRecoverChoice::DiscardAutosaveOnly;
    }

    return AutosaveRecoverChoice::KeepForLater;
  }
}

void setupAutosaveTimer(OpenSCADApp *app)
{
  auto *timer = new QTimer(app);
  const int initialSeconds = Settings::Settings::autosaveSessionIntervalSeconds.value();
  timer->setInterval(std::max(10, initialSeconds) * 1000);

  struct AutosaveTimerState final : QObject {
    explicit AutosaveTimerState(QObject *parent) : QObject(parent) {}
    uint64_t state = 0;
    bool performed = false;
  };
  auto *timerState = new AutosaveTimerState(timer);

  QObject::connect(timer, &QTimer::timeout, app, [timer, timerState]() {
    const int intervalSeconds = Settings::Settings::autosaveSessionIntervalSeconds.value();
    const int intervalMs = std::max(10, intervalSeconds) * 1000;
    if (timer->interval() != intervalMs) {
      timer->setInterval(intervalMs);
    }

    const bool sessionMgmt = Settings::Settings::sessionManagementEnabled.value();
    const bool enabled = sessionMgmt && Settings::Settings::autosaveSessionEnabled.value();

    if (!enabled) {
      if (timerState->performed) {
        QFile::remove(TabManager::getAutosaveFilePath());
        timerState->performed = false;
        timerState->state = 0;
      }
      return;
    }

    const bool dirty = TabManager::hasDirtyTabs();
    if (!dirty) {
      if (timerState->performed) {
        QFile::remove(TabManager::getAutosaveFilePath());
        timerState->performed = false;
        timerState->state = 0;
      }
      return;
    }

    const uint64_t generation = TabManager::sessionDirtyGeneration();
    if (!timerState->performed || generation != timerState->state) {
      TabManager::saveGlobalSession(TabManager::getAutosaveFilePath());
      timerState->state = generation;
      timerState->performed = true;
    }
  });

  timer->start();
}

bool saveSessionForShutdown()
{
  if (!Settings::Settings::sessionManagementEnabled.value()) return false;
  const auto& windows = scadApp->windowManager.getWindows();
  if (windows.isEmpty()) {
    return false;
  }
  if (TabManager::shouldSkipSessionSave()) {
    QFile::remove(TabManager::getAutosaveFilePath());
    return false;
  }
  for (auto *win : windows) {
    win->markSessionQuitting();
  }
  QString saveError;
  const bool success =
    TabManager::saveGlobalSession(TabManager::getSessionFilePath(), &saveError, false);
  if (!success && !saveError.isEmpty()) {
    LOG(message_group::UI_Warning, "Failed to save session on shutdown: %1$s",
        saveError.toUtf8().constData());
  }
  if (success) {
    QFile::remove(TabManager::getAutosaveFilePath());
  }
  return success;
}

constexpr int kIpcTimeoutMs = 1500;

QString lockFilePath()
{
  const QString baseDir = TabManager::getSessionFilePath();
  return QFileInfo(baseDir).absolutePath() + QStringLiteral("/pythonscad.lock");
}

QString serverNameFromPath(const QString& path)
{
  const QByteArray hash = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex();
  return QStringLiteral("pythonscad.instance.") + QString::fromUtf8(hash);
}

QString serverName()
{
  return serverNameFromPath(lockFilePath());
}

QString resolveOpenMode(const std::string& overrideMode)
{
  if (overrideMode == "new-window" || overrideMode == "active-window") {
    return QString::fromStdString(overrideMode);
  }
  return QString::fromStdString(Settings::Settings::singleInstanceOpenMode.value());
}

void applyLocalizationAndGuiThemeForEarlyDialogs(OpenSCADApp& application)
{
  QSettingsCached s;
  if (s.value("advanced/localization", true).toBool()) {
    localization_init();
  }
  application.setGuiTheme(GlobalPreferences::inst()->getValue("advanced/guiTheme").toString());
}

bool lockHostnameLooksLocal(const QString& hostname)
{
  if (hostname.isEmpty()) {
    return true;
  }
  if (hostname.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
    return true;
  }
  return hostname.compare(QHostInfo::localHostName(), Qt::CaseInsensitive) == 0;
}

bool terminateProcessIdForSingleInstance(qint64 pid)
{
  if (pid <= 0) {
    return true;
  }
#ifdef Q_OS_UNIX
  const auto p = static_cast<pid_t>(pid);
  if (kill(p, 0) == -1 && errno == ESRCH) {
    return true;
  }
  /* A process stopped with SIGSTOP does not act on SIGTERM until SIGCONT (see signal(7)). */
  (void)kill(p, SIGCONT);
  return kill(p, SIGTERM) == 0;
#elif defined(Q_OS_WIN)
  const HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
  if (!h) {
    return false;
  }
  const BOOL ok = TerminateProcess(h, 1);
  CloseHandle(h);
  return ok != FALSE;
#else
  (void)pid;
  return false;
#endif
}

bool tryAcquirePrimaryLockAfterCloseOtherRequest(QLockFile& lock)
{
  qint64 pid = 0;
  QString hostname, appname;
  const bool hasInfo = lock.getLockInfo(&pid, &hostname, &appname);
  if (hasInfo && pid > 0 && lockHostnameLooksLocal(hostname)) {
    (void)terminateProcessIdForSingleInstance(pid);
    for (int i = 0; i < 50; ++i) {
      QThread::msleep(100);
      lock.removeStaleLockFile();
      if (lock.tryLock()) {
        return true;
      }
    }
#ifdef Q_OS_UNIX
    /* If the instance ignores SIGTERM or is wedged, force kill so the user can start a primary. */
    const auto p = static_cast<pid_t>(pid);
    if (kill(p, 0) == 0) {
      (void)kill(p, SIGCONT);
      (void)kill(p, SIGKILL);
      for (int j = 0; j < 40; ++j) {
        QThread::msleep(50);
        lock.removeStaleLockFile();
        if (lock.tryLock()) {
          return true;
        }
      }
    }
#endif
    return false;
  }
  lock.removeStaleLockFile();
  return lock.tryLock();
}

void focusWindow(MainWindow *window)
{
  if (!window) return;
  window->show();
  window->raise();
  window->activateWindow();
}

void openFilesInWindow(MainWindow *window, const QStringList& files)
{
  if (!window) return;
  for (const auto& file : files) {
    if (file.isEmpty()) continue;
    window->tabManager->open(file);
  }
  focusWindow(window);
}

MainWindow *getOrCreateActiveWindow()
{
  const auto& wins = scadApp->windowManager.getWindows();
  MainWindow *tracked = scadApp->windowManager.getLastActive();
  if (tracked && wins.contains(tracked)) {
    return tracked;
  }
  for (MainWindow *mw : wins) {
    if (mw && mw->isActiveWindow()) {
      return mw;
    }
  }
  for (QWidget *w = QApplication::activeWindow(); w != nullptr; w = w->parentWidget()) {
    if (auto *mw = qobject_cast<MainWindow *>(w)) {
      if (wins.contains(mw)) {
        return mw;
      }
    }
  }
  if (!wins.isEmpty()) {
    return *wins.begin();
  }
  return nullptr;
}

QJsonObject buildIpcMessage(const QString& action, const QStringList& files, const QString& openMode,
                            const QString& cwd)
{
  QJsonObject obj;
  obj.insert(QStringLiteral("action"), action);
  obj.insert(QStringLiteral("openMode"), openMode);
  obj.insert(QStringLiteral("cwd"), cwd);
  QJsonArray fileArray;
  for (const auto& file : files) fileArray.append(file);
  obj.insert(QStringLiteral("files"), fileArray);
  return obj;
}

bool sendIpcMessage(const QJsonObject& message)
{
  QLocalSocket socket;
  socket.connectToServer(serverName());
  if (!socket.waitForConnected(kIpcTimeoutMs)) {
    return false;
  }

  QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
  payload.append('\n');
  socket.write(payload);
  if (!socket.waitForBytesWritten(kIpcTimeoutMs)) {
    return false;
  }
  socket.flush();
  if (!socket.waitForReadyRead(kIpcTimeoutMs)) {
    return false;
  }
  const QByteArray ack = socket.readAll();
  socket.disconnectFromServer();
  return ack.startsWith("ok");
}

void finishIpcClient(QLocalSocket *socket)
{
  socket->flush();
  socket->disconnectFromServer();
  socket->deleteLater();
}

// Handle one newline-delimited JSON frame. Returns false after writing "error\n" (caller closes).
// Returns true after writing "ok\n" (caller may process more frames on the same connection).
bool dispatchIpcLine(QLocalSocket *socket, const QByteArray& data)
{
  const auto doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) {
    socket->write("error\n");
    return false;
  }
  const auto obj = doc.object();
  const auto action = obj.value(QStringLiteral("action")).toString();
  const auto openMode = obj.value(QStringLiteral("openMode")).toString();
  const auto filesValue = obj.value(QStringLiteral("files"));

  if (action == QStringLiteral("focus")) {
    focusWindow(getOrCreateActiveWindow());
    socket->write("ok\n");
    return true;
  }

  if (action != QStringLiteral("open") || !filesValue.isArray()) {
    socket->write("error\n");
    return false;
  }

  QStringList files;
  for (const auto& entry : filesValue.toArray()) {
    const auto path = entry.toString();
    if (!path.isEmpty()) files.append(path);
  }

  if (files.isEmpty()) {
    focusWindow(getOrCreateActiveWindow());
    socket->write("ok\n");
    return true;
  }

  if (openMode == QStringLiteral("active-window")) {
    auto *active = getOrCreateActiveWindow();
    if (active) {
      openFilesInWindow(active, files);
    } else {
      new MainWindow(files);
    }
    socket->write("ok\n");
    return true;
  }

  new MainWindow(files);
  socket->write("ok\n");
  return true;
}

void startIpcServer(QLocalServer *server)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
  // Restrict access to the same user where supported (mitigates cross-user open/focus on shared hosts).
  server->setSocketOptions(QLocalServer::UserAccessOption);
#endif

  QObject::connect(server, &QLocalServer::newConnection, [server]() {
    while (auto *socket = server->nextPendingConnection()) {
      socket->setProperty("_ipc_buf", QByteArray());
      QObject::connect(socket, &QLocalSocket::readyRead, [socket]() {
        QByteArray buf = socket->property("_ipc_buf").toByteArray();
        buf.append(socket->readAll());
        bool dispatchedOk = false;
        for (;;) {
          const int nlPos = buf.indexOf('\n');
          if (nlPos < 0) {
            socket->setProperty("_ipc_buf", buf);
            if (dispatchedOk && buf.isEmpty()) {
              finishIpcClient(socket);
            }
            return;
          }
          const QByteArray line = buf.left(nlPos);
          buf = buf.mid(nlPos + 1);
          if (line.isEmpty()) {
            continue;
          }
          if (!dispatchIpcLine(socket, line)) {
            socket->setProperty("_ipc_buf", QByteArray());
            finishIpcClient(socket);
            return;
          }
          dispatchedOk = true;
        }
      });
    }
  });

  if (!server->listen(serverName())) {
    if (server->serverError() == QAbstractSocket::AddressInUseError) {
      QLocalServer::removeServer(serverName());
      if (!server->listen(serverName())) {
        LOG(message_group::UI_Warning,
            "Could not start the local IPC server after reclaiming the socket name "
            "(single-instance open/focus may not work): %1$s",
            server->errorString().toUtf8().constData());
      }
    } else {
      LOG(message_group::UI_Warning, "Could not start the local IPC server: %1$s",
          server->errorString().toUtf8().constData());
    }
  }
}

#ifdef Q_OS_UNIX
int shutdownSignalPipe[2] = {-1, -1};

void shutdownSignalHandler(int)
{
  const char signalByte = 1;
  if (shutdownSignalPipe[1] != -1) {
    (void)::write(shutdownSignalPipe[1], &signalByte, sizeof(signalByte));
  }
}

void setupUnixSignalHandlers(OpenSCADApp *app)
{
  if (::pipe(shutdownSignalPipe) != 0) return;
  // Both ends non-blocking: the notifier slot drains with a read loop; a blocking read
  // after the pipe is empty would hang the GUI thread (no EOF while the write end stays open).
  for (int fd : {shutdownSignalPipe[0], shutdownSignalPipe[1]}) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
      fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    const int fdFlags = fcntl(fd, F_GETFD, 0);
    if (fdFlags != -1) {
      fcntl(fd, F_SETFD, fdFlags | FD_CLOEXEC);
    }
  }

  auto *notifier = new QSocketNotifier(shutdownSignalPipe[0], QSocketNotifier::Read, app);
  QObject::connect(notifier, &QSocketNotifier::activated, app, [notifier](int) {
    notifier->setEnabled(false);
    char buffer[32];
    for (;;) {
      const ssize_t n = ::read(shutdownSignalPipe[0], buffer, sizeof(buffer));
      if (n <= 0) break;
    }
    saveSessionForShutdown();
    QCoreApplication::quit();
    notifier->setEnabled(true);
  });

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = shutdownSignalHandler;
  sigaction(SIGTERM, &action, nullptr);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGHUP, &action, nullptr);
}
#endif

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

void dialogThreadFunc(FontCacheInitializer *initializer)
{
  initializer->run();
}

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
void registerDefaultIcon(const QString&)
{
}
#endif

}  // namespace

#ifdef OPENSCAD_SUFFIX
#define DESKTOP_FILENAME "pythonscad" OPENSCAD_SUFFIX
#else
#define DESKTOP_FILENAME "pythonscad"
#endif

int gui(std::vector<std::string>& inputFiles, const std::filesystem::path& original_path, int argc,
        char **argv, const std::string& gui_test, const bool reset_window_settings,
        const std::string& open_in_override)
{
  configureOpenGLContext();
  OpenSCADApp app(argc, argv);

  // set up groups for QSettings
  QCoreApplication::setOrganizationName("PythonSCAD");
  QCoreApplication::setOrganizationDomain("pythonscad.org");
  QCoreApplication::setApplicationName("PythonSCAD");
  QCoreApplication::setApplicationVersion(QString::fromStdString(std::string(openscad_versionnumber)));
  QGuiApplication::setApplicationDisplayName("PythonSCAD");
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

  applyLocalizationAndGuiThemeForEarlyDialogs(app);

  const QString openMode = resolveOpenMode(open_in_override);
  const QString cwd = QString::fromStdString(original_path.generic_string());

  QLockFile lock(lockFilePath());
  lock.setStaleLockTime(0);
  if (!lock.tryLock()) {
    const QStringList ipcFiles = [&]() {
      QStringList files;
      for (const auto& infile : inputFiles) {
        if (!infile.empty()) {
          files.append(assemblePath(original_path, infile));
        }
      }
      return files;
    }();
    const QString action = ipcFiles.isEmpty() ? QStringLiteral("focus") : QStringLiteral("open");
    const QJsonObject message = buildIpcMessage(action, ipcFiles, openMode, cwd);

    if (sendIpcMessage(message)) {
      return 0;
    }

    while (true) {
      QMessageBox box;
      box.setIcon(QMessageBox::Warning);
      box.setWindowTitle(_("PythonSCAD"));
      box.setText(
        _("PythonSCAD is already running but is not responding. The old PythonSCAD process must be "
          "closed to open a new window."));
      box.setInformativeText(
        _("Try again if the running instance may have recovered, or close it to start a new one."));
      auto *retryBtn = box.addButton(_("Retry"), QMessageBox::AcceptRole);
      auto *closeOtherBtn = box.addButton(_("Close PythonSCAD"), QMessageBox::ActionRole);
      auto *cancelBtn = box.addButton(_("Cancel"), QMessageBox::RejectRole);
      box.setDefaultButton(retryBtn);
      box.exec();

      QAbstractButton *clicked = box.clickedButton();
      if (clicked == cancelBtn || clicked == nullptr) {
        return 1;
      }
      if (clicked == retryBtn) {
        if (sendIpcMessage(message)) {
          return 0;
        }
        continue;
      }

      if (tryAcquirePrimaryLockAfterCloseOtherRequest(lock)) {
        break;
      }
      if (sendIpcMessage(message)) {
        return 0;
      }
    }
  }

  QLocalServer ipcServer;
  startIpcServer(&ipcServer);

  QSettingsCached settings;
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

#ifdef OPENSCAD_UPDATER
  AutoUpdater *updater = new SparkleAutoUpdater;
  AutoUpdater::setUpdater(updater);
  if (updater->automaticallyChecksForUpdates()) updater->checkForUpdates();
  updater->init();
#endif

  QObject::connect(GlobalPreferences::inst(), &Preferences::applicationFontChanged, &app,
                   &OpenSCADApp::setApplicationFont);
  QObject::connect(GlobalPreferences::inst(), &Preferences::guiThemeChanged, &app,
                   &OpenSCADApp::setGuiTheme);
  QObject::connect(GlobalPreferences::inst(), &Preferences::renderBackend3DChanged, &app,
                   &OpenSCADApp::setRenderBackend3D);

  set_render_color_scheme(arg_colorscheme, false);
  auto noInputFiles = false;

  const bool sessionMgmtEnabled = Settings::Settings::sessionManagementEnabled.value();

  if (!inputFiles.size()) {
    noInputFiles = true;
    inputFiles.emplace_back("");
  }

  QStringList inputFilesList;
  for (const auto& infile : inputFiles) {
    inputFilesList.append(assemblePath(original_path, infile));
  }

  QVector<QStringList> windowsToOpen;
  QStringList filesToAppend;
  bool restoreSessionForExplicitFiles = false;
  bool openedSessionWindows = false;

  if (sessionMgmtEnabled && noInputFiles) {
    const QString sessionPath = TabManager::getSessionFilePath();
    const QString autosavePath = TabManager::getAutosaveFilePath();
    if (shouldOfferAutosaveRestore(autosavePath, sessionPath)) {
      switch (promptAutosaveRestore(autosavePath)) {
      case AutosaveRecoverChoice::RestoreIntoSession: {
        QFile in(autosavePath);
        QSaveFile out(sessionPath);
        if (in.open(QIODevice::ReadOnly) && out.open(QIODevice::WriteOnly)) {
          out.write(in.readAll());
          const bool committed = out.commit();
          if (committed) {
            if (!QFile::setPermissions(sessionPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
              LOG(message_group::UI_Warning, "Failed to set session file permissions: %1$s",
                  sessionPath.toUtf8().constData());
            }
            QFile::remove(autosavePath);
          }
        } else {
          out.cancelWriting();
        }
      } break;
      case AutosaveRecoverChoice::DiscardAutosaveOnly: QFile::remove(autosavePath); break;
      case AutosaveRecoverChoice::StartFreshSession:
        QFile::remove(autosavePath);
        QFile::remove(sessionPath);
        break;
      case AutosaveRecoverChoice::KeepForLater: break;
      }
    }
  }

  const bool hasExplicitFiles = !(inputFilesList.size() == 1 && inputFilesList[0].isEmpty());

  if (sessionMgmtEnabled) {
    const QString sessionPath = TabManager::getSessionFilePath();
    const bool sessionExists = QFileInfo(sessionPath).exists();
    const bool shouldRestore = sessionExists && !TabManager::sessionHasOnlyEmptyTab(sessionPath);
    if (shouldRestore) {
      openedSessionWindows = true;
      const int windowCount = TabManager::sessionWindowCount(sessionPath);
      if (windowCount > 0) {
        for (int i = 0; i < windowCount; ++i) {
          windowsToOpen.append(QStringList(QStringLiteral(":session:%1:").arg(i)));
        }
      } else {
        windowsToOpen.append(QStringList(QStringLiteral(":session:")));
      }
      if (hasExplicitFiles) {
        restoreSessionForExplicitFiles = true;
        filesToAppend = inputFilesList;
      }
    }
  }

  if (windowsToOpen.isEmpty()) {
    // Show launcher only when no files and no session to restore
    if (noInputFiles && inputFilesList.size() == 1 && inputFilesList[0].isEmpty()) {
      auto showOnStartup = settings.value("launcher/showOnStartup");
      if (showOnStartup.isNull() || showOnStartup.toBool()) {
        LaunchingScreen launcher;
        if (launcher.exec() == QDialog::Accepted) {
          if (launcher.isForceShowEditor()) {
            settings.setValue("view/hideEditor", false);
          }
          const QStringList files = launcher.selectedFiles();
          if (!files.empty()) {
            inputFilesList.clear();
            for (const auto& f : files) {
              inputFilesList.append(assemblePath(original_path, f.toStdString()));
            }
          }
        } else {
          return 0;
        }
      }
    }
    windowsToOpen.append(inputFilesList);
  }

  QVector<MainWindow *> openedMainWindows;
  openedMainWindows.reserve(windowsToOpen.size());
  for (const auto& files : windowsToOpen) {
    openedMainWindows.append(new MainWindow(files));
  }

  if (openedSessionWindows && !openedMainWindows.isEmpty()) {
    const QString sessionPath = TabManager::getSessionFilePath();
    const int sessionWindows = TabManager::sessionWindowCount(sessionPath);
    if (sessionWindows > 0 && openedMainWindows.size() == sessionWindows) {
      const int activeIdx = TabManager::sessionActiveWindowIndex(sessionPath);
      scadApp->windowManager.setLastActive(openedMainWindows[activeIdx]);
    }
  }

  if (restoreSessionForExplicitFiles && !filesToAppend.isEmpty()) {
    const bool openInNewWindow = openMode == QStringLiteral("new-window");
    if (openInNewWindow) {
      new MainWindow(filesToAppend);
    } else {
      MainWindow *target = getOrCreateActiveWindow();
      if (!target) {
        new MainWindow(filesToAppend);
      } else {
        openFilesInWindow(target, filesToAppend);
      }
    }
  }

  setupAutosaveTimer(&app);

  QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
    saveSessionForShutdown();
    QSettingsCached{}.release();
#ifdef Q_OS_MACOS
    CocoaUtils::endApplication();
#endif
  });

  QObject::connect(&app, &QGuiApplication::commitDataRequest, &app,
                   [](QSessionManager&) { saveSessionForShutdown(); });
  QObject::connect(&app, &QGuiApplication::saveStateRequest, &app,
                   [](QSessionManager&) { saveSessionForShutdown(); });

#ifdef Q_OS_UNIX
  setupUnixSignalHandlers(&app);
#endif

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
