#pragma once

#include <QObject>
#include <memory>

#include "utils/printutils.h"

class Tree;

class GeometryWorker : public QObject
{
  Q_OBJECT;

public:
  GeometryWorker(LogClient *logClient);
  ~GeometryWorker() override;

public slots:
  void start(const Tree& tree);

protected slots:
  void work();

signals:
  void done(std::shared_ptr<const class Geometry>);

protected:
  class QThread *thread;
  const Tree *tree = nullptr;

private:
  LogClient *logClient;
};
