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
  void startPreview(const QString& source, const QString& filename);
  void cancel();

protected slots:
  void processOutput();

signals:
  void done(std::shared_ptr<const class Geometry>);
  void previewDone(const QString& source);

protected:
  class QProcess *process;
  class QTemporaryFile *sourceFile;
  QString resultPath;
  enum class Request { NONE, RENDER, PREVIEW } request = Request::NONE;
  bool busy = false;
  bool stopping = false;
  void startProcess();
  void startRequest(const QString& command, const QString& suffix, const QString& source,
                    const QString& filename);
};
