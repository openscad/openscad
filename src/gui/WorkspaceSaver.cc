#include "WorkspaceSaver.h"
#include <QApplication>
#include <QEvent>
#include <QSettings>
#include "gui/QSettingsCached.h"
#include <QSessionManager>

/**
 * @class WorkspaceSaver
 * @brief Singleton class responsible for safely capturing and saving the application's workspace state.
 *
 * This class ensures that the layout state (including the state of floating QDockWidgets)
 * is safely captured and saved to a configuration file upon application exit. It handles
 * scenarios like multiple windows, quit interruptions, and the arbitrary closing order
 * of top-level floating docks.
 */

WorkspaceSaver *WorkspaceSaver::instance()
{
  static WorkspaceSaver *inst = nullptr;
  if (!inst) {
    inst = new WorkspaceSaver(qApp);
  }
  return inst;
}

WorkspaceSaver::WorkspaceSaver(QObject *parent) : QObject(parent)
{
  // aboutToQuit is emitted after closeAllWindows() has been called, so we can be sure that the
  // geometry/state we capture is the last one before the app quits.
  connect(qApp, &QCoreApplication::aboutToQuit, this, &WorkspaceSaver::commitToSettings);
  // commitDataRequest is emitted when the user tries to quit the app, but before closeAllWindows() is
  // called.
  connect(qApp, &QGuiApplication::commitDataRequest, this, [this](QSessionManager&) {
    captureActiveMainWindow();
    lock();
  });
  qApp->installEventFilter(this);
}

/**
 * Finds the currently active main window and captures its state.
 */
void WorkspaceSaver::captureActiveMainWindow()
{
  QMainWindow *activeMainWindow = nullptr;
  for (QWidget *widget : QApplication::topLevelWidgets()) {
    if (auto *win = qobject_cast<QMainWindow *>(widget)) {
      activeMainWindow = win;
      if (win->isActiveWindow()) {
        break;
      }
    }
  }
  if (activeMainWindow) {
    captureState(activeMainWindow);
  }
}

void WorkspaceSaver::captureState(QMainWindow *window)
{
  if (locked_) return;
  saved_geometry_ = window->saveGeometry();
  saved_state_ = window->saveState();
}

/**
 * This should be called once a valid state is guaranteed to be captured,
 * avoiding subsequent overwrites by partial state destructions during shutdown.
 */
void WorkspaceSaver::lock()
{
  locked_ = true;
}

/**
 *
 * This is typically used when a quit operation is aborted (e.g., user cancels).
 */
void WorkspaceSaver::unlock()
{
  locked_ = false;
}

bool WorkspaceSaver::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Quit) {
    // By the time Quit is processed, closeAllWindows() has already hidden docks.
    // We only capture here if we haven't locked it (i.e. if it wasn't captured in closeEvent).
    if (!locked_ && saved_geometry_.isEmpty()) {
      captureActiveMainWindow();
      lock();
    }
  }
  return QObject::eventFilter(obj, event);
}

/**
 * @brief Commits the currently captured state and geometry to persistent settings.
 */
void WorkspaceSaver::commitToSettings()
{
  if (!saved_geometry_.isEmpty() || !saved_state_.isEmpty()) {
    QSettingsCached settings;
    if (!saved_geometry_.isEmpty()) {
      settings.setValue("window/geometry", saved_geometry_);
    }
    if (!saved_state_.isEmpty()) {
      settings.setValue("window/state", saved_state_);
    }
  }
}