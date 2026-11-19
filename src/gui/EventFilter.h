#pragma once

#include <QFileOpenEvent>
#include <QObject>

#include "gui/LaunchingScreen.h"
#include "gui/OpenSCADApp.h"
#include "gui/UIUtils.h"

class SCADEventFilter : public QObject
{
  Q_OBJECT;

public:
  SCADEventFilter(QObject *parent) : QObject(parent) {}

protected:
  bool eventFilter(QObject *obj, QEvent *event) override
  {
    // Handle Apple event for opening files, only available on OS X
    if (event->type() == QEvent::FileOpen) {
      QFileOpenEvent *foe = static_cast<QFileOpenEvent *>(event);
      const fs::path& filename = UIUtils::QStringToPath(foe->file());
      if (LaunchingScreen *ls = LaunchingScreen::getDialog()) {
        // We need to invoke the method since, apparently, we receive
        // this event in another thread.
        QMetaObject::invokeMethod(ls, "openFile", Qt::QueuedConnection, Q_ARG(fs::path, filename));
      } else {
        scadApp->requestOpenFile(filename);
      }
      return true;
    } else {
      // standard event processing
      return QObject::eventFilter(obj, event);
    }
  }
};
