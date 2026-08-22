#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <vector>
#include <memory>

#include "core/customizer/ParameterSet.h"
#include "utils/printutils.h"

#include <deque>

struct RequestContext {
  struct Diagnostic {
    Message message;
    QString shape;
    size_t count = 1;
  };

  enum class Type { RENDER, PREVIEW } type = Type::PREVIEW;
  quint64 id = 0;
  std::shared_ptr<class QTemporaryDir> requestDirectory;
  std::shared_ptr<class QTemporaryFile> sourceFile;
  std::shared_ptr<class QTemporaryFile> parameterFile;
  QString resultPath;
  QString requestSource;
  QString displayFilename;
  bool canceled = false;
  bool diagnosticsEnded = false;
  bool collapseDiagnostics = true;
  QString unabridgedDiagnostics;
  std::vector<Diagnostic> diagnostics;
};

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
  void output(const Message& message);
  void unabridgedOutput(const QString& text);
  void progress(int permille);
  void parametersDiscovered(const QString& source, const QString& metadata);
  void dependenciesDiscovered(const QString& source, const QStringList& dependencies);

protected:
  class QProcess *process;
  QMetaObject::Connection startErrorConnection;
  QString program;
  int consecutiveFailures = 0;
  QByteArray pendingRequest;
  // How many activeRequests entries are still buffered in pendingRequest, i.e. queued
  // but not yet written to the worker process.
  size_t pendingCount = 0;
  quint64 nextRequestId = 0;
  QByteArray standardErrorBuffer;
  enum class Request { NONE, RENDER, PREVIEW } request = Request::NONE;
  std::deque<std::shared_ptr<RequestContext>> activeRequests;
  // Most recently completed request, kept only so its temporary filename can
  // still be rewritten out of diagnostics that arrive after its response.
  std::shared_ptr<RequestContext> lastRetiredRequest;
  bool ready = false;
  bool busy = false;
  bool canceled = false;
  bool stopping = false;
  static void cleanupResult(const QString& resultPath);
  void processMetadata(const std::shared_ptr<RequestContext>& req);
  void processStandardError();
  void startProcess();
  void flushPendingRequests();
  void updateBusyState();
  void startRequest(const QString& command, const QString& suffix, const QString& source,
                    const QString& filename, const ParameterSet& parameters, size_t normalizationLimit,
                    double time, const Camera& camera, bool python, const QString& pythonVenv);
};
