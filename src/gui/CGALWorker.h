#pragma once

#include <QObject>
#include <memory>

class Tree;

class CGALWorker : public QObject
{
  Q_OBJECT;

public:
  CGALWorker();
  ~CGALWorker() override;
  qint64 processId() const;

public slots:
  void start(const Tree& tree);

protected slots:
  void work();

signals:
  void done(std::shared_ptr<const class Geometry>);

protected:
  class QThread *thread;
  class QProcess *process;
  const class Tree *tree;
};
