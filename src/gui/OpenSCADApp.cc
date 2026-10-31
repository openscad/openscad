#include "gui/OpenSCADApp.h"

#include "gui/MainWindow.h"
#ifdef Q_OS_MACOS
#include "gui/EventFilter.h"
#endif

#include "geometry/GeometryCache.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALCache.h"
#endif
#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QPalette>
#include <QProgressDialog>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <boost/foreach.hpp>
#include <cassert>
#include <exception>

#include "glview/RenderSettings.h"

namespace {

QtMessageHandler defaultMessageHandler = nullptr;

void logHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
  if (msg.contains("Using Qt multimedia with FFmpeg")) return;

  if (defaultMessageHandler) defaultMessageHandler(type, ctx, msg);
}

// Dark palette used with the Fusion style. Disabled roles are set explicitly,
// since Fusion would otherwise derive them from the light defaults.
QPalette darkPalette()
{
  const QColor window(45, 45, 48);
  const QColor base(30, 30, 30);
  const QColor text(220, 220, 220);
  const QColor disabledText(127, 127, 127);
  const QColor highlight(42, 130, 218);

  QPalette p;
  p.setColor(QPalette::Window, window);
  p.setColor(QPalette::WindowText, text);
  p.setColor(QPalette::Base, base);
  p.setColor(QPalette::AlternateBase, window);
  p.setColor(QPalette::ToolTipBase, base);
  p.setColor(QPalette::ToolTipText, text);
  p.setColor(QPalette::PlaceholderText, disabledText);
  p.setColor(QPalette::Text, text);
  p.setColor(QPalette::Button, window);
  p.setColor(QPalette::ButtonText, text);
  p.setColor(QPalette::BrightText, QColor(255, 80, 80));
  p.setColor(QPalette::Light, QColor(70, 70, 74));
  p.setColor(QPalette::Midlight, QColor(60, 60, 64));
  p.setColor(QPalette::Mid, QColor(40, 40, 42));
  p.setColor(QPalette::Dark, QColor(25, 25, 25));
  p.setColor(QPalette::Shadow, QColor(0, 0, 0));
  p.setColor(QPalette::Link, highlight.lighter(130));
  p.setColor(QPalette::LinkVisited, QColor(170, 130, 230));
  p.setColor(QPalette::Highlight, highlight);
  p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
    p.setColor(QPalette::Disabled, role, disabledText);
  }
  p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
  p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
  return p;
}

}  // namespace

OpenSCADApp::OpenSCADApp(int& argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACOS
  this->installEventFilter(new SCADEventFilter(this));
#endif

  // Wire the log filter early in the app creation process
  defaultMessageHandler = qInstallMessageHandler(logHandler);

  // Note: It may be tempting to add more initialization code here, but keep in mind that this is run as
  // part of QApplication initialization, so it's usually better to that in the main gui() function after
  // the OpenSCADApp instance is created.
}

OpenSCADApp::~OpenSCADApp()
{
  delete this->fontCacheDialog;
}

#include <QMessageBox>

bool OpenSCADApp::notify(QObject *object, QEvent *event)
{
  QString msg;
  try {
    return QApplication::notify(object, event);
  } catch (const std::exception& e) {
    msg = e.what();
  } catch (...) {
    msg = _("Unknown error");
  }
  // This happens when an uncaught exception is thrown in a Qt event handler
  QMessageBox::critical(
    nullptr, QString(_("Critical Error")),
    QString(_("A critical error was caught. The application may have become unstable:\n%1"))
      .arg(QString(msg)));
  return false;
}

/*!
   Requests to open a file from an external event, e.g. by double-clicking a filename.
 */
void OpenSCADApp::requestOpenFile(const QString& filename)
{
  for (auto win : this->windowManager.getWindows()) {
    // if we have an empty open window, use that one
    if (win->isEmpty()) {
      win->tabManager->createTab(filename);
      return;
    }
  }

  // ..otherwise, create a new one
  new MainWindow(QStringList(filename));
}

void OpenSCADApp::showFontCacheDialog()
{
  if (!this->fontCacheDialog) this->fontCacheDialog = new QProgressDialog();
  this->fontCacheDialog->setLabelText(
    _("Fontconfig needs to update its font cache.\nThis can take up to a couple of minutes."));
  this->fontCacheDialog->setMinimum(0);
  this->fontCacheDialog->setMaximum(0);
  this->fontCacheDialog->setCancelButton(nullptr);
  this->fontCacheDialog->exec();
}

void OpenSCADApp::hideFontCacheDialog()
{
  assert(this->fontCacheDialog);
  this->fontCacheDialog->reset();
}

void OpenSCADApp::setRenderBackend3D(RenderBackend3D backend)
{
  RenderSettings::inst()->backend3D = backend;
  CGALCache::instance()->clear();
  GeometryCache::instance()->clear();
  emit backend3DChanged();
}

void OpenSCADApp::setUiTheme(const QString& theme)
{
  // Remember what the platform gave us, so "Follow system" can go back to it.
  static const QString nativeStyle = QApplication::style()->objectName();
  static const QPalette nativePalette = QApplication::palette();
  static bool overridden = false;

  // Since Qt 6.8 the color scheme can be overridden per application, which
  // also keeps native dialogs and the title bar in sync with the choice.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  styleHints()->setColorScheme(theme == "dark"    ? Qt::ColorScheme::Dark
                               : theme == "light" ? Qt::ColorScheme::Light
                                                  : Qt::ColorScheme::Unknown);
#endif

  if (theme == "dark") {
    // Fusion honors a custom palette consistently on every platform, unlike
    // the native Windows and macOS styles.
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setPalette(darkPalette());
    overridden = true;
  } else if (theme == "light") {
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setPalette(QApplication::style()->standardPalette());
    overridden = true;
  } else if (overridden) {
    // Only restore when a forced theme was applied before, so the platform
    // palette is never pinned and live OS theme changes keep working.
    QApplication::setStyle(QStyleFactory::create(nativeStyle));
    QApplication::setPalette(nativePalette);
    overridden = false;
  }
}

void OpenSCADApp::setApplicationFont(const QString& family, uint size)
{
  // Update the application font (hopefully) everywhere.
  scadApp->setFont(QFont(family, size));
}

// For Qt5, simulate the Qt6 behavior.
//
// https://doc.qt.io/qt-6/qtcore-changes-qt6.html#other-classes
//
//     In Qt 5, QCoreApplication::quit() was equivalent to calling QCoreApplication::exit().
//     This just exited the main event loop.
//
//     In Qt 6, the method will instead try to close all top-level windows by posting a close
//     event. The windows are free to cancel the shutdown process by ignoring the event.
//
// But this implements a slightly modified Qt6 behavior as that triggers
// deletion of all widget windows before calling MainWindow::closeEvent.
// We really just call close on our own list of main windows.
void OpenSCADApp::closeApp()
{
  for (MainWindow *mw : scadApp->windowManager.getWindows()) {
    if (!mw->close()) {
      return;
    }
  }
  // Should not be reached, when closing the last MainWindow,
  // the quit() call would already happen in the close event
  // handler MainWindow::closeEvent().
  QApplication::quit();
}
