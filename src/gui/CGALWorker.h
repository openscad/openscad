#pragma once

#include <QObject>
#include "memory.h"

class Tree;

class CGALWorker : public QObject
{
  Q_OBJECT;
public:
  CGALWorker();
  ~CGALWorker() override;

public slots:
  void start(const Tree& tree);

protected slots:
  void work();

signals:
  void done(shared_ptr<const class Geometry>);

protected:

  class QThread *thread;
  const class Tree *tree;
};
