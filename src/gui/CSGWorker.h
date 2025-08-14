#pragma once

#include <QObject>
#include <memory>

#include "MainWindow.h"
class CSGWorker : public QObject
{
  Q_OBJECT;

public:
  CSGWorker(MainWindow *main);
  ~CSGWorker() override;

public slots:
  int start();

protected slots:
  void work();

signals:
  void done(void);

protected:
  class QThread *thread;
  MainWindow *main;
  int started;
};
