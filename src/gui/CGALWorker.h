#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "core/customizer/ParameterSet.h"

class CGALWorker : public QObject
{
  Q_OBJECT;

public:
  CGALWorker();
  ~CGALWorker() override;
  qint64 processId() const;
  void start(const QString& source, const QString& filename, const ParameterSet& parameters, double time,
             const class Camera& camera);
  void startPreview(const QString& source, const QString& filename, const ParameterSet& parameters,
                    size_t normalizationLimit, double time, const Camera& camera);

public slots:
  void cancel();

protected slots:
  void processOutput();

signals:
  void done(std::shared_ptr<const class Geometry>);
  void previewDone(std::shared_ptr<class CsgInfo> products);
  void diagnostic(const QString& text);
  void parametersDiscovered(const QString& source, const QString& metadata);
  void dependenciesDiscovered(const QString& source, const QStringList& dependencies);

protected:
  class QProcess *process;
  class QTemporaryFile *sourceFile;
  class QTemporaryFile *parameterFile;
  QString resultPath;
  QString displayFilename;
  QString requestSource;
  enum class Request { NONE, RENDER, PREVIEW } request = Request::NONE;
  bool busy = false;
  bool stopping = false;
  void cleanupResult();
  void processMetadata();
  void startProcess();
  void startRequest(const QString& command, const QString& suffix, const QString& source,
                    const QString& filename, const ParameterSet& parameters, size_t normalizationLimit,
                    double time, const Camera& camera);
};
