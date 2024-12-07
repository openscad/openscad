#pragma once

#include <QObject>
#include <memory>
#include <optional>


class Tree;

class CGALWorker : public QObject
{
  Q_OBJECT;
public:
  CGALWorker();
  ~CGALWorker() override;

public slots:
  void start(const Tree& tree, const int counter);

protected slots:
  void work();

signals:
  void done(std::shared_ptr<const class Geometry>, const int counter);

protected:

  class QThread *thread;
  const class Tree *tree;
  int counter;
};
