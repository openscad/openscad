#pragma once

#include <QFileOpenEvent>
#include <QObject>

#include "gui/LaunchingScreen.h"
#include "gui/OpenSCADApp.h"

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
      const QString& filename = foe->file();
      QMetaObject::invokeMethod(scadApp, "handleOpenFileEvent", Qt::QueuedConnection,
                                Q_ARG(QString, filename));
      return true;
    } else {
      // standard event processing
      return QObject::eventFilter(obj, event);
    }
  }
};
