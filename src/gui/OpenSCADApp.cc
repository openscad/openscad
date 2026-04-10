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
#include <QProgressDialog>
#include <QString>
#include <QStringList>
#include <boost/foreach.hpp>
#include <cassert>
#include <exception>

#include "glview/RenderSettings.h"

OpenSCADApp::OpenSCADApp(int& argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACOS
  this->installEventFilter(new SCADEventFilter(this));
#endif

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
}

void OpenSCADApp::setApplicationFont(const QString& family, uint size)
{
  QApplication::setFont(QFont(family, size));
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// https://doc.qt.io/qt-6/qtcore-changes-qt6.html#other-classes
//
//     In Qt 5, QCoreApplication::quit() was equivalent to calling QCoreApplication::exit().
//     This just exited the main event loop.
//
//     In Qt 6, the method will instead try to close all top-level windows by posting a close
//     event. The windows are free to cancel the shutdown process by ignoring the event.
//
// For Qt5, simulate the Qt6 behavior.
void OpenSCADApp::quit()
{
  for (MainWindow *mw : scadApp->windowManager.getWindows()) {
    if (!mw->close()) {
      return;
    }
  }
  QApplication::quit();
}
#endif
