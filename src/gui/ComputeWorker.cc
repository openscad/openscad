#include "gui/ComputeWorker.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QTimer>
#include <algorithm>
#include <filesystem>
#include <memory>

#include "Feature.h"
#include "core/AST.h"
#include "core/customizer/ParameterSet.h"
#include "geometry/PolySet.h"
#include "glview/Camera.h"
#include "glview/CsgInfo.h"
#include "glview/RenderSettings.h"
#include "io/import.h"
#include "io/ipc_geometry.h"
#include "openscad.h"
#include "utils/printutils.h"

ComputeWorker::ComputeWorker(const QString& program) : program(program)
{
  this->process = new QProcess();
  connect(this->process, &QProcess::readyReadStandardOutput, this, &ComputeWorker::processOutput);
  connect(this->process, &QProcess::readyReadStandardError, this, &ComputeWorker::processStandardError);
  connect(this->process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [this](int, QProcess::ExitStatus) {
            if (this->stopping) return;
            this->ready = false;
            // Requests already written to the dead process are lost with it. Requests still
            // buffered in pendingRequest have not been written yet and will be resent to the
            // replacement worker, so their contexts must stay on the deque. Dropping that
            // distinction desynchronizes activeRequests from the worker permanently.
            const auto pending = std::min(this->pendingCount, this->activeRequests.size());
            const auto lost = this->activeRequests.size() - pending;
            const auto interrupted = lost > 0;
            const auto request = this->request;
            if (interrupted) {
              this->activeRequests.erase(this->activeRequests.begin(),
                                         this->activeRequests.begin() + lost);
              updateBusyState();
            }
            if (++this->consecutiveFailures <= 3) {
              QTimer::singleShot(100 * this->consecutiveFailures, this, &ComputeWorker::startProcess);
            } else {
              emit diagnostic(tr("Compute worker stopped after repeated failures."));
              if (!interrupted && this->busy) {
                this->pendingRequest.clear();
                this->pendingCount = 0;
                this->busy = false;
                this->request = Request::NONE;
                this->activeRequests.clear();
                if (request == Request::PREVIEW) emit previewDone({});
                else emit done({});
              }
            }
            if (interrupted && request == Request::PREVIEW) emit previewDone({});
            else if (interrupted) emit done({});
          });
  startProcess();
}

void ComputeWorker::processStandardError()
{
  standardErrorBuffer += this->process->readAllStandardError();
  while (standardErrorBuffer.contains('\n')) {
    auto line = QString::fromUtf8(standardErrorBuffer.left(standardErrorBuffer.indexOf('\n'))).trimmed();
    standardErrorBuffer.remove(0, standardErrorBuffer.indexOf('\n') + 1);
    // stdout and stderr are separate channels, so a request's trailing diagnostics
    // can arrive after its "done" has already popped it off the queue. Rewriting
    // against the front request alone therefore leaks the worker's temporary
    // filename into the console whenever that race is lost — substitute every
    // request whose name could still be in flight.
    auto rewrite = [&line](const std::shared_ptr<RequestContext>& req) {
      if (!req || !req->sourceFile) return;
      line.replace(req->sourceFile->fileName(), req->displayFilename);
      line.replace(QFileInfo(req->sourceFile->fileName()).fileName(),
                   QFileInfo(req->displayFilename).fileName());
    };
    rewrite(this->lastRetiredRequest);
    for (const auto& req : this->activeRequests) rewrite(req);
    auto group = message_group::NONE;
    if (line.startsWith("ECHO:")) group = message_group::Echo;
    else if (line.startsWith("WARNING:")) group = message_group::Warning;
    else if (line.startsWith("ERROR:")) group = message_group::Error;
    if (!line.isEmpty()) emit output(Message(line.toStdString(), group));
  }
}

void ComputeWorker::startProcess()
{
  this->ready = false;
  // A worker killed mid-payload leaves a half-read frame and a non-zero byte count behind. The
  // replacement's first response would be consumed as the remainder of that payload, so the
  // response stream has to start clean -- this is the respawn path a crash recovery goes through.
  this->outputBuffer.clear();
  this->payloadRemaining = 0;
  this->payloadReader = {};
  this->pendingPayloads.clear();
  disconnect(this->startErrorConnection);
  this->startErrorConnection =
    connect(this->process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
      if (error != QProcess::FailedToStart) return;
      disconnect(this->startErrorConnection);
      emit diagnostic(tr("Could not start compute worker: %1").arg(this->process->errorString()));
    });
  this->process->start(this->program.isEmpty() ? QCoreApplication::applicationFilePath() : this->program,
                       {"--compute-worker"});
}

ComputeWorker::~ComputeWorker()
{
  this->stopping = true;
  this->process->write("quit\n");
  if (!this->process->waitForFinished(1000)) {
    this->process->terminate();
    if (!this->process->waitForFinished(1000)) this->process->kill();
    this->process->waitForFinished();
  }
  delete this->process;
  for (const auto& req : this->activeRequests) {
    if (req) cleanupResult(req->resultPath);
  }
  this->activeRequests.clear();
}

void ComputeWorker::cleanupResult(const QString& resultPath)
{
  if (resultPath.isEmpty()) return;
  // Only the cancel flag is still a file. The results -- geometry, products, per-leaf payloads
  // and the metadata sidecars -- come back over the response channel and were never written
  // (feature 32), so there is nothing else here to remove.
  QFile::remove(resultPath + ".cancel");
}

qint64 ComputeWorker::processId() const
{
  return this->process->processId();
}

#ifdef ENABLE_GUI_TESTS
void ComputeWorker::exitForTest()
{
  this->process->write("exit-for-test\n");
}
#endif

void ComputeWorker::start(const QString& source, const QString& filename, const ParameterSet& parameters,
                          double time, const Camera& camera, bool python, const QString& pythonVenv)
{
  startRequest("render", ".off", source, filename, parameters, 0, time, camera, python, pythonVenv);
}

void ComputeWorker::startPreview(const QString& source, const QString& filename,
                                 const ParameterSet& parameters, size_t normalizationLimit, double time,
                                 const Camera& camera, bool python, const QString& pythonVenv)
{
  startRequest("preview", ".csg", source, filename, parameters, normalizationLimit, time, camera, python,
               pythonVenv);
}

void ComputeWorker::startRequest(const QString& command, const QString& suffix, const QString& source,
                                 const QString& filename, const ParameterSet& parameters,
                                 size_t normalizationLimit, double time, const Camera& camera,
                                 bool python, const QString& pythonVenv)
{
  this->canceled = false;
  auto req = std::make_shared<RequestContext>();
  req->type = command == "preview" ? RequestContext::Type::PREVIEW : RequestContext::Type::RENDER;
  req->requestDirectory = std::make_shared<QTemporaryDir>(QDir::tempPath() + "/openscad-worker-XXXXXX");
  const auto directory = req->requestDirectory->path();
  const auto workingDirectory =
    filename.isEmpty() ? QDir::currentPath() : QFileInfo(filename).absolutePath();
  req->displayFilename = filename.isEmpty() ? QString("Untitled.scad") : filename;
  req->sourceFile = std::make_shared<QTemporaryFile>(directory + "/.openscad-worker-XXXXXX.scad");
  if (!req->sourceFile->open()) {
    LOG(message_group::Error, "Could not create compute worker input: %1$s",
        req->sourceFile->errorString().toStdString());
    if (command == "preview") emit previewDone({});
    else emit done({});
    return;
  }
  req->sourceFile->write(source.toUtf8());
  if (!python) {
    req->sourceFile->write("\n\x03\n");
    req->sourceFile->write(QByteArray::fromStdString(commandline_commands));
  }
  req->sourceFile->flush();
  req->sourceFile->close();
  req->requestSource = source;
  QString parameterPath;
  if (!parameters.empty()) {
    req->parameterFile = std::make_shared<QTemporaryFile>(directory + "/.openscad-worker-XXXXXX.json");
    if (!req->parameterFile->open()) {
      if (command == "preview") emit previewDone({});
      else emit done({});
      return;
    }
    parameterPath = req->parameterFile->fileName();
    req->parameterFile->close();
    ParameterSets sets;
    sets.push_back(parameters);
    sets.writeFile(parameterPath.toStdString());
  }
  req->resultPath = req->sourceFile->fileName() + suffix;
  this->request = command == "preview" ? Request::PREVIEW : Request::RENDER;
  this->busy = true;
  this->activeRequests.push_back(req);

  const auto vpr = camera.getVpr();
  const auto vpt = camera.getVpt();
  QJsonObject request{
    {"command", command},
    {"input", req->sourceFile->fileName()},
    {"output", req->resultPath},
    {"workingDirectory", workingDirectory},
    {"sourcePath", filename.isEmpty() ? workingDirectory + "/Untitled.scad" : filename},
    {"parameterFile", parameterPath},
    {"setName", "worker"},
    {"normalizationLimit", static_cast<qint64>(normalizationLimit)},
    {"colorscheme", QString::fromStdString(RenderSettings::inst()->colorscheme)},
    {"time", time}};
  QJsonArray cameraValues;
  for (const auto value :
       {vpr.x(), vpr.y(), vpr.z(), vpt.x(), vpt.y(), vpt.z(), camera.zoomValue(), camera.fovValue()}) {
    cameraValues.append(value);
  }
  request["camera"] = cameraValues;
  QJsonArray features;
  for (const auto feature : boost::make_iterator_range(Feature::begin(), Feature::end())) {
    if (feature->is_enabled()) features.append(QString::fromStdString(feature->get_name()));
  }
  request["features"] = features;
  request["python"] = python;
  request["pythonVenv"] = pythonVenv;
  // Append, never assign: activeRequests is a queue, so a second request started while
  // the worker is not ready yet (startup, or the respawn window) must not replace the
  // first. Losing one leaves an orphan context that misaligns every later response.
  this->pendingRequest += QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";
  ++this->pendingCount;
  flushPendingRequests();
}

void ComputeWorker::updateBusyState()
{
  this->busy = !this->activeRequests.empty();
  if (!this->busy) {
    this->request = Request::NONE;
    return;
  }
  this->request = this->activeRequests.front()->type == RequestContext::Type::PREVIEW ? Request::PREVIEW
                                                                                      : Request::RENDER;
}

void ComputeWorker::flushPendingRequests()
{
  if (!this->ready || this->pendingRequest.isEmpty()) return;
  this->process->write(this->pendingRequest);
  this->pendingRequest.clear();
  this->pendingCount = 0;
}

void ComputeWorker::cancel()
{
  this->canceled = true;
  for (const auto& req : this->activeRequests) {
    if (req) {
      req->canceled = true;
      QFile cancelFile(req->resultPath + ".cancel");
      cancelFile.open(QIODevice::WriteOnly);
    }
  }
  if (!this->busy) return;
  if (!this->activeRequests.empty()) {
    const auto activePath = this->activeRequests.front()->resultPath;
    QTimer::singleShot(1000, this, [this, activePath] {
      if (!this->busy || this->activeRequests.empty() ||
          this->activeRequests.front()->resultPath != activePath)
        return;
      this->process->terminate();
    });
  }
}

void ComputeWorker::processMetadata(const std::shared_ptr<RequestContext>& req,
                                    const IpcPayloadResolver& resolve)
{
  if (!req) return;
  if (const auto *parameters = resolve((req->resultPath + ".parameters.json").toStdString())) {
    emit parametersDiscovered(req->requestSource, QString::fromStdString(*parameters));
  }
  if (const auto *dependencies = resolve((req->resultPath + ".dependencies.json").toStdString())) {
    QStringList paths;
    for (const auto& path : QJsonDocument::fromJson(QByteArray::fromStdString(*dependencies)).array()) {
      paths.push_back(path.toString());
    }
    emit dependenciesDiscovered(req->requestSource, paths);
  }
}

void ComputeWorker::processOutput()
{
  this->outputBuffer += this->process->readAll();
  while (true) {
    // Payload mode: consume exactly the announced byte count, whatever it contains. Doing this
    // by line would split a payload at the first newline in the mesh data.
    if (this->payloadRemaining > 0) {
      const auto take = std::min<qint64>(this->payloadRemaining, this->outputBuffer.size());
      if (take == 0) return;
      this->payloadReader.append(this->outputBuffer.constData(), static_cast<size_t>(take));
      this->outputBuffer.remove(0, static_cast<int>(take));
      this->payloadRemaining -= take;
      if (this->payloadRemaining == 0) {
        IpcMessage message;
        while (this->payloadReader.next(message)) {
          this->pendingPayloads[ipc_payload_name(message.name)] = std::move(message.payload);
        }
        if (this->payloadReader.failed()) {
          LOG(message_group::Error, "Compute worker sent a malformed payload.");
        }
      }
      continue;
    }

    const auto newline = this->outputBuffer.indexOf('\n');
    if (newline < 0) return;
    const auto response = this->outputBuffer.left(newline).trimmed();
    this->outputBuffer.remove(0, newline + 1);

    if (response.startsWith("payload\t")) {
      bool valid = false;
      const auto announced = response.mid(8).toLongLong(&valid);
      // An unparseable or implausible count would otherwise leave the reader waiting for bytes
      // that never come, which looks exactly like a worker that has hung. Same ceiling the
      // framing itself enforces, for the same reason.
      if (!valid || announced < 0 || static_cast<quint64>(announced) > kIpcMaxMessageSize) {
        LOG(message_group::Error, "Compute worker announced an unusable payload size '%1$s'.",
            response.mid(8).toStdString());
        this->process->kill();
        return;
      }
      this->payloadRemaining = announced;
      continue;
    }
    if (response == "ready") {
      this->consecutiveFailures = 0;
      this->ready = true;
      flushPendingRequests();
      continue;
    }
    if (response == "pong") continue;
    if (response.startsWith("progress\t")) {
      emit progress(response.mid(9).toInt());
    } else if (response == "done" || response == "previewdone" || response == "error" ||
               response == "cancelled") {
      std::shared_ptr<RequestContext> req;
      if (this->activeRequests.empty()) {
        // The worker answered a request we have no record of: the queue and the worker
        // have drifted apart, which shows up as a stale or blank preview.
        LOG(message_group::Error,
            "Compute worker sent an unexpected '%1$s' response with no request outstanding.",
            response.toStdString());
      } else {
        req = this->activeRequests.front();
        this->lastRetiredRequest = req;
        this->activeRequests.pop_front();
      }
      updateBusyState();

      // Payloads arrive ahead of the response that terminates their request, so whatever has
      // accumulated belongs to this one. Taken by move so a failed request cannot leave its
      // payloads behind to be misread as the next one's.
      const auto payloads = std::exchange(this->pendingPayloads, {});
      // Normalised on lookup exactly as the sink normalises on write, so the two ends agree
      // regardless of which spelling of the path each of them was handed.
      const IpcPayloadResolver resolve = [&payloads](const std::string& name) -> const std::string * {
        const auto found = payloads.find(ipc_payload_name(name));
        return found == payloads.end() ? nullptr : &found->second;
      };

      if (req) {
        processMetadata(req, resolve);
        if (response == "done") {
          const auto *payload = resolve(req->resultPath.toStdString());
          std::unique_ptr<PolySet> geometry;
          if (payload) {
            geometry = import_ipc_geometry_buffer(payload->data(), payload->size(),
                                                  req->resultPath.toStdString());
          } else {
            LOG(message_group::Error, "Compute worker returned no geometry for the render.");
          }
          emit done(std::shared_ptr<const Geometry>(std::move(geometry)));
        } else if (response == "previewdone") {
          if (!req->canceled) {
            auto products = std::make_shared<CsgInfo>();
            if (!products->read_products((req->resultPath + ".products.json").toStdString(),
                                         [this, req]() {
                                           QCoreApplication::processEvents();
                                           return !req->canceled;
                                         },
                                         resolve)) {
              products.reset();
            }
            emit previewDone(std::move(products));
          } else {
            emit previewDone({});
          }
        } else {
          if (req->type == RequestContext::Type::PREVIEW) emit previewDone({});
          else emit done({});
        }
        cleanupResult(req->resultPath);
      }
    }
  }
}
