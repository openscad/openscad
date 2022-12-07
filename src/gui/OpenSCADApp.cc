#include "OpenSCADApp.h"
#include "MainWindow.h"
#ifdef Q_OS_MAC
#include "EventFilter.h"
#endif

#include <QProgressDialog>
#include <iostream>
#include <boost/foreach.hpp>
#include "QSettingsCached.h"

OpenSCADApp::OpenSCADApp(int& argc, char **argv)
  : QApplication(argc, argv)
{
#ifdef Q_OS_MAC
  this->installEventFilter(new SCADEventFilter(this));
#endif
}

OpenSCADApp::~OpenSCADApp()
{
  delete this->fontCacheDialog;
}

#include <QMessageBox>

// See: https://bugreports.qt.io/browse/QTBUG-65592
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
void OpenSCADApp::workaround_QTBUG_65592(QObject *o, QEvent *e)
{
  QMainWindow *mw;
  if (o->isWidgetType() && e->type() == QEvent::MouseButtonPress && (mw = qobject_cast<QMainWindow *>(o))) {
    for (auto& ch : mw->children()) {
      if (auto dw = qobject_cast<QDockWidget *>(ch)) {
        auto pname = "_wa-QTBUG-65592";
        auto v = dw->property(pname);
        if (v.isNull()) {
          dw->setProperty(pname, true);
          mw->restoreDockWidget(dw);
          auto area = mw->dockWidgetArea(dw);
          auto orient = area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea ? Qt::Horizontal : Qt::Vertical;
          mw->resizeDocks({dw}, {orient == Qt::Horizontal ? dw->width() : dw->height() }, orient);
        }
      }
    }
  }
}
#else
void OpenSCADApp::workaround_QTBUG_65592(QObject *, QEvent *) { }
#endif // if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))

bool OpenSCADApp::notify(QObject *object, QEvent *event)
{
  QString msg;
  try {
    workaround_QTBUG_65592(object, event);
    return QApplication::notify(object, event);
  } catch (const std::exception& e) {
    msg = e.what();
  } catch (...) {
    msg = _("Unknown error");
  }
  // This happens when an uncaught exception is thrown in a Qt event handler
  QMessageBox::critical(nullptr, QString(_("Critical Error")), QString(_("A critical error was caught. The application may have become unstable:\n%1")).arg(QString(msg)));
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
  this->fontCacheDialog->setLabelText(_("Fontconfig needs to update its font cache.\nThis can take up to a couple of minutes."));
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


void OpenSCADApp::releaseQSettingsCached() {
  QSettingsCached{}.release();
}
