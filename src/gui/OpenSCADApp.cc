#include "gui/OpenSCADApp.h"
#include "gui/MainWindow.h"
#ifdef Q_OS_MACOS
#include "gui/EventFilter.h"
#endif

#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QString>
#include <QStringList>
#include <cassert>
#include <exception>
#include <QProgressDialog>
#include <boost/foreach.hpp>

OpenSCADApp::OpenSCADApp(int& argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACOS
  this->installEventFilter(new SCADEventFilter(this));
#endif
}

OpenSCADApp::~OpenSCADApp() { delete this->fontCacheDialog; }

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

void OpenSCADApp::setApplicationFont(const QString& family, uint size)
{
  // Trigger style sheet refresh to update the application font
  // (hopefully) everywhere. Also remove ugly frames in the QStatusBar
  // when using additional widgets
  const auto stylesheet = QString(R"(
    * {
        font-family: '%1';
        font-size: %2pt;
    }
    QStatusBar::item {
        border: 0px solid black;
    }
  )");
  scadApp->setStyleSheet(stylesheet.arg(family, QString::number(size)));
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
