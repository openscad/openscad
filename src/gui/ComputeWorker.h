#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <memory>

#include "core/customizer/ParameterSet.h"
#include "io/ipc_channel.h"
#include "utils/printutils.h"

#include <deque>
#include <map>
#include <string>

struct RequestContext {
  enum class Type { RENDER, PREVIEW } type = Type::PREVIEW;
  std::shared_ptr<class QTemporaryDir> requestDirectory;
  std::shared_ptr<class QTemporaryFile> sourceFile;
  std::shared_ptr<class QTemporaryFile> parameterFile;
  QString resultPath;
  QString requestSource;
  QString displayFilename;
  // Snapshotted at dispatch, never re-read from the global feature when the response arrives: a
  // preview already running keeps the setting it started with even if the user toggles the flag
  // while it is in flight.
  bool streaming = false;
  bool canceled = false;
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
  QByteArray standardErrorBuffer;
  // Responses are read as bytes rather than by line: a payload shares this stream with the
  // control lines and contains newlines of its own, so the reader switches to counting bytes
  // for the length a "payload" line announces (feature 32).
  QByteArray outputBuffer;
  qint64 payloadRemaining = 0;
  IpcMessageReader payloadReader;
  // Payloads received since the last terminating response, keyed by the path the worker would
  // have written. Attached to a request when its "done"/"previewdone" arrives, because that is
  // what identifies which request they belonged to.
  std::map<std::string, std::string> pendingPayloads;
  // Leaves decoded as they arrived, so the decode overlaps the worker's remaining evaluation
  // instead of happening all at once after `previewdone` (feature 34).
  std::map<std::string, std::shared_ptr<const class PolySet>> decodedLeaves;
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
  void processMetadata(const std::shared_ptr<RequestContext>& req, const IpcPayloadResolver& resolve);
  void processStandardError();
  void startProcess();
  void flushPendingRequests();
  void updateBusyState();
  void startRequest(const QString& command, const QString& suffix, const QString& source,
                    const QString& filename, const ParameterSet& parameters, size_t normalizationLimit,
                    double time, const Camera& camera, bool python, const QString& pythonVenv);
};
