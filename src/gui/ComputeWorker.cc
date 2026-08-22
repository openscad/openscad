#include "gui/ComputeWorker.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
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
#include "gui/Preferences.h"
#include "io/import.h"
#include "io/ipc_geometry.h"
#include "openscad.h"
#include "utils/printutils.h"

namespace {

message_group messageGroup(const QString& group)
{
  if (group == "error") return message_group::Error;
  if (group == "warning") return message_group::Warning;
  if (group == "echo") return message_group::Echo;
  if (group == "trace") return message_group::Trace;
  if (group == "deprecated") return message_group::Deprecated;
  if (group == "parser-error") return message_group::Parser_Error;
  if (group == "ui-error") return message_group::UI_Error;
  if (group == "ui-warning") return message_group::UI_Warning;
  if (group == "font-warning") return message_group::Font_Warning;
  if (group == "export-error") return message_group::Export_Error;
  if (group == "export-warning") return message_group::Export_Warning;
  if (group == "html-link") return message_group::HtmlLink;
  return message_group::NONE;
}

bool isCollapsible(const message_group group)
{
  return group == message_group::Warning || group == message_group::Error ||
         group == message_group::Deprecated || group == message_group::Parser_Error ||
         group == message_group::UI_Error || group == message_group::UI_Warning ||
         group == message_group::Export_Error || group == message_group::Export_Warning;
}

QString diagnosticShape(QString message)
{
  static const QRegularExpression number(
    R"((?<![A-Za-z_])[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?(?![A-Za-z_]))");
  return message.replace(number, "#");
}

}  // namespace

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
  QFile::remove(resultPath);
  QFile::remove(resultPath + ".parameters.json");
  QFile::remove(resultPath + ".dependencies.json");
  QFile::remove(resultPath + ".cancel");
  const auto products = resultPath + ".products.json";
  QFile::remove(products);
  for (size_t index = 0;
       QFile::remove(products + ".leaf-" + QString::number(index) + kIpcGeometrySuffix); ++index) {
  }
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
  req->id = ++this->nextRequestId;
  req->collapseDiagnostics =
    GlobalPreferences::inst()->getValue("advanced/collapseDiagnostics").toBool();
  req->collapseDiagnosticFamilies = Feature::ExperimentalDiagnosticFamilies.is_enabled();
  req->type = command == "preview" ? RequestContext::Type::PREVIEW : RequestContext::Type::RENDER;
  req->requestDirectory = std::make_shared<QTemporaryDir>(QDir::tempPath() + "/openscad-worker-XXXXXX");
  const auto directory = req->requestDirectory->path();
  const auto sourceInfo = QFileInfo(filename);
  auto workingDirectory = filename.isEmpty() ? QDir::currentPath() : sourceInfo.absolutePath();
  if (!QDir(workingDirectory).exists()) workingDirectory = QDir::homePath();
  const auto sourcePath =
    filename.isEmpty() || !QDir(sourceInfo.absolutePath()).exists()
      ? QDir(workingDirectory).filePath(filename.isEmpty() ? "Untitled.scad" : sourceInfo.fileName())
      : filename;
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
    {"requestId", static_cast<qint64>(req->id)},
    {"command", command},
    {"input", req->sourceFile->fileName()},
    {"output", req->resultPath},
    {"workingDirectory", workingDirectory},
    {"sourcePath", sourcePath},
    {"parameterFile", parameterPath},
    {"setName", "worker"},
    {"normalizationLimit", static_cast<qint64>(normalizationLimit)},
    {"colorscheme", QString::fromStdString(RenderSettings::inst()->colorscheme)},
    // The worker does the evaluating, so it is the worker's caches that have to be the size the
    // user configured; it has no access to the preferences itself.
    {"polysetCacheSizeMB",
     static_cast<qint64>(GlobalPreferences::inst()->getValue("advanced/polysetCacheSizeMB").toUInt())},
    {"cgalCacheSizeMB",
     static_cast<qint64>(GlobalPreferences::inst()->getValue("advanced/cgalCacheSizeMB").toUInt())},
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

void ComputeWorker::processMetadata(const std::shared_ptr<RequestContext>& req)
{
  if (!req) return;
  QFile parameters(req->resultPath + ".parameters.json");
  if (parameters.open(QIODevice::ReadOnly)) {
    emit parametersDiscovered(req->requestSource, QString::fromUtf8(parameters.readAll()));
  }
  QFile dependencies(req->resultPath + ".dependencies.json");
  if (dependencies.open(QIODevice::ReadOnly)) {
    QStringList paths;
    for (const auto& path : QJsonDocument::fromJson(dependencies.readAll()).array()) {
      paths.push_back(path.toString());
    }
    emit dependenciesDiscovered(req->requestSource, paths);
  }
}

void ComputeWorker::processOutput()
{
  while (this->process->canReadLine()) {
    const auto response = this->process->readLine().trimmed();
    if (response == "ready") {
      this->consecutiveFailures = 0;
      this->ready = true;
      flushPendingRequests();
      continue;
    }
    if (response == "pong") continue;
    if (response.startsWith("progress\t")) {
      emit progress(response.mid(9).toInt());
    } else if (response.startsWith("diagnostic\t")) {
      const auto record = QJsonDocument::fromJson(response.mid(11)).object();
      const auto requestId = static_cast<quint64>(record["requestId"].toDouble());
      const auto request = std::find_if(this->activeRequests.begin(), this->activeRequests.end(),
                                        [requestId](const std::shared_ptr<RequestContext>& candidate) {
                                          return candidate && candidate->id == requestId;
                                        });
      if (request == this->activeRequests.end()) continue;

      const auto location = record["location"].toObject();
      auto filename = location["file"].toString();
      if ((*request)->sourceFile && filename == (*request)->sourceFile->fileName()) {
        filename = (*request)->displayFilename;
      }
      auto messageText = record["message"].toString();
      if ((*request)->sourceFile) {
        messageText.replace((*request)->sourceFile->fileName(), (*request)->displayFilename);
      }
      auto path = filename.isEmpty() ? std::shared_ptr<std::filesystem::path>{}
                                     : std::make_shared<std::filesystem::path>(filename.toStdString());
      Message message(messageText.toStdString(), messageGroup(record["group"].toString()),
                      Location(location["line"].toInt(), location["column"].toInt(),
                               location["lastLine"].toInt(), location["lastColumn"].toInt(), path),
                      (*request)->displayFilename.toStdString());
      (*request)->unabridgedDiagnostics += QString::fromStdString(message.str()) + '\n';

      const auto collapsible = isCollapsible(message.group);
      if (!collapsible || !(*request)->collapseDiagnostics) {
        emit output(message);
        continue;
      }
      const auto shape =
        (*request)->collapseDiagnosticFamilies ? diagnosticShape(messageText) : messageText;
      const auto duplicate =
        std::find_if((*request)->diagnostics.begin(), (*request)->diagnostics.end(),
                     [&message, &shape](const RequestContext::Diagnostic& candidate) {
                       return candidate.message.group == message.group &&
                              candidate.message.loc.fileName() == message.loc.fileName() &&
                              candidate.message.loc.firstLine() == message.loc.firstLine() &&
                              candidate.shape == shape;
                     });
      if (duplicate != (*request)->diagnostics.end()) {
        ++duplicate->count;
      } else {
        (*request)->diagnostics.push_back({message, shape, 1});
        emit output(message);
      }
    } else if (response.startsWith("diagnostics-end\t")) {
      const auto requestId = response.mid(16).toULongLong();
      const auto request = std::find_if(this->activeRequests.begin(), this->activeRequests.end(),
                                        [requestId](const std::shared_ptr<RequestContext>& candidate) {
                                          return candidate && candidate->id == requestId;
                                        });
      if (request == this->activeRequests.end()) continue;
      (*request)->diagnosticsEnded = true;
      emit unabridgedOutput((*request)->unabridgedDiagnostics);
      for (const auto& diagnostic : (*request)->diagnostics) {
        if (diagnostic.count < 2) continue;
        emit output(
          Message(diagnostic.message.msg + " (occurred " + std::to_string(diagnostic.count) + " times)",
                  diagnostic.message.group, diagnostic.message.loc, diagnostic.message.docPath,
                  diagnostic.count - 1));
      }
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

      if (req) {
        processMetadata(req);
        if (response == "done") {
          auto geometry = import_ipc_geometry(req->resultPath.toStdString());
          emit done(std::shared_ptr<const Geometry>(std::move(geometry)));
        } else if (response == "previewdone") {
          if (!req->canceled) {
            auto products = std::make_shared<CsgInfo>();
            if (!products->read_products((req->resultPath + ".products.json").toStdString(),
                                         [this, req]() {
                                           QCoreApplication::processEvents();
                                           return !req->canceled;
                                         })) {
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
