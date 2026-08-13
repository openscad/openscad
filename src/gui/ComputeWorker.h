#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <memory>

#include "core/customizer/ParameterSet.h"

class ComputeWorker : public QObject
{
  Q_OBJECT;

public:
  explicit ComputeWorker(const QString& program = {});
  ~ComputeWorker() override;
  qint64 processId() const;
#ifdef ENABLE_GUI_TESTS
  void exitForTest();
#endif
  void start(const QString& source, const QString& filename, const ParameterSet& parameters, double time,
             const class Camera& camera, bool python, const QString& pythonVenv);
  void startPreview(const QString& source, const QString& filename, const ParameterSet& parameters,
                    size_t normalizationLimit, double time, const Camera& camera, bool python,
                    const QString& pythonVenv);

public slots:
  void cancel();

protected slots:
  void processOutput();

signals:
  void done(std::shared_ptr<const class Geometry>);
  void previewDone(std::shared_ptr<class CsgInfo> products);
  void diagnostic(const QString& text);
  void progress(int permille);
  void parametersDiscovered(const QString& source, const QString& metadata);
  void dependenciesDiscovered(const QString& source, const QStringList& dependencies);

protected:
  class QProcess *process;
  class QTemporaryFile *sourceFile;
  class QTemporaryFile *parameterFile;
  class QTemporaryDir *requestDirectory;
  QMetaObject::Connection startErrorConnection;
  QString program;
  int consecutiveFailures = 0;
  QString resultPath;
  QString displayFilename;
  QString requestSource;
  QByteArray pendingRequest;
  enum class Request { NONE, RENDER, PREVIEW } request = Request::NONE;
  bool ready = false;
  bool busy = false;
  bool stopping = false;
  void cleanupResult();
  void processMetadata();
  void startProcess();
  void startRequest(const QString& command, const QString& suffix, const QString& source,
                    const QString& filename, const ParameterSet& parameters, size_t normalizationLimit,
                    double time, const Camera& camera, bool python, const QString& pythonVenv);
};
