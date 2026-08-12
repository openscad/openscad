#pragma once

#include <QObject>
#include <QString>
#include <memory>

class CGALWorker : public QObject
{
  Q_OBJECT;

public:
  CGALWorker();
  ~CGALWorker() override;
  qint64 processId() const;

public slots:
  void start(const QString& source, const QString& filename);
  void cancel();

protected slots:
  void processOutput();

signals:
  void done(std::shared_ptr<const class Geometry>);

protected:
  class QProcess *process;
  class QTemporaryFile *sourceFile;
  QString resultPath;
  void startProcess();
};
