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
#include <filesystem>
#include <memory>

#include "core/AST.h"
#include "core/customizer/ParameterSet.h"
#include "geometry/PolySet.h"
#include "glview/Camera.h"
#include "glview/CsgInfo.h"
#include "io/import.h"
#include "openscad.h"
#include "utils/printutils.h"

ComputeWorker::ComputeWorker(const QString& program) : program(program)
{
  this->sourceFile = nullptr;
  this->parameterFile = nullptr;
  this->requestDirectory = nullptr;
  this->process = new QProcess();
  connect(this->process, &QProcess::readyReadStandardOutput, this, &ComputeWorker::processOutput);
  connect(this->process, &QProcess::readyReadStandardError, this, [this] {
    auto text = QString::fromUtf8(this->process->readAllStandardError());
    if (this->sourceFile) {
      text.replace(this->sourceFile->fileName(), this->displayFilename);
      text.replace(QFileInfo(this->sourceFile->fileName()).fileName(),
                   QFileInfo(this->displayFilename).fileName());
    }
    emit diagnostic(text.trimmed());
  });
  connect(this->process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [this](int, QProcess::ExitStatus) {
            if (this->stopping) return;
            this->ready = false;
            const auto interrupted = this->busy && this->pendingRequest.isEmpty();
            const auto request = this->request;
            if (interrupted) {
              this->busy = false;
              this->request = Request::NONE;
            }
            if (++this->consecutiveFailures <= 3) {
              QTimer::singleShot(100 * this->consecutiveFailures, this, &ComputeWorker::startProcess);
            } else {
              emit diagnostic(tr("Compute worker stopped after repeated failures."));
              if (!interrupted && this->busy) {
                this->pendingRequest.clear();
                this->busy = false;
                this->request = Request::NONE;
                if (request == Request::PREVIEW) emit previewDone({});
                else emit done({});
              }
            }
            if (interrupted && request == Request::PREVIEW) emit previewDone({});
            else if (interrupted) emit done({});
          });
  startProcess();
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
  cleanupResult();
  delete this->sourceFile;
  delete this->parameterFile;
  delete this->requestDirectory;
}

void ComputeWorker::cleanupResult()
{
  if (this->resultPath.isEmpty()) return;
  QFile::remove(this->resultPath);
  QFile::remove(this->resultPath + ".parameters.json");
  QFile::remove(this->resultPath + ".dependencies.json");
  QFile::remove(this->resultPath + ".cancel");
  const auto products = this->resultPath + ".products.json";
  QFile::remove(products);
  for (size_t index = 0; QFile::remove(products + ".leaf-" + QString::number(index) + ".off"); ++index) {
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
  cleanupResult();
  delete this->sourceFile;
  delete this->parameterFile;
  delete this->requestDirectory;
  this->parameterFile = nullptr;
  this->requestDirectory = new QTemporaryDir(QDir::tempPath() + "/openscad-worker-XXXXXX");

  const auto directory = this->requestDirectory->path();
  const auto workingDirectory =
    filename.isEmpty() ? QDir::currentPath() : QFileInfo(filename).absolutePath();
  this->displayFilename = filename.isEmpty() ? QString("Untitled.scad") : filename;
  this->sourceFile = new QTemporaryFile(directory + "/.openscad-worker-XXXXXX.scad");
  if (!this->sourceFile->open()) {
    LOG(message_group::Error, "Could not create compute worker input: %1$s",
        this->sourceFile->errorString().toStdString());
    if (command == "preview") emit previewDone({});
    else emit done({});
    return;
  }
  this->sourceFile->write(source.toUtf8());
  if (!python) {
    this->sourceFile->write("\n\x03\n");
    this->sourceFile->write(QByteArray::fromStdString(commandline_commands));
  }
  this->sourceFile->flush();
  this->requestSource = source;
  QString parameterPath;
  if (!parameters.empty()) {
    this->parameterFile = new QTemporaryFile(directory + "/.openscad-worker-XXXXXX.json");
    if (!this->parameterFile->open()) {
      if (command == "preview") emit previewDone({});
      else emit done({});
      return;
    }
    parameterPath = this->parameterFile->fileName();
    this->parameterFile->close();
    ParameterSets sets;
    sets.push_back(parameters);
    sets.writeFile(parameterPath.toStdString());
  }
  this->resultPath = this->sourceFile->fileName() + suffix;
  this->request = command == "preview" ? Request::PREVIEW : Request::RENDER;
  this->busy = true;
  const auto vpr = camera.getVpr();
  const auto vpt = camera.getVpt();
  QJsonObject request{{"command", command},
                      {"input", this->sourceFile->fileName()},
                      {"output", this->resultPath},
                      {"workingDirectory", workingDirectory},
                      {"parameterFile", parameterPath},
                      {"setName", "worker"},
                      {"normalizationLimit", static_cast<qint64>(normalizationLimit)},
                      {"time", time}};
  QJsonArray cameraValues;
  for (const auto value :
       {vpr.x(), vpr.y(), vpr.z(), vpt.x(), vpt.y(), vpt.z(), camera.zoomValue(), camera.fovValue()}) {
    cameraValues.append(value);
  }
  request["camera"] = cameraValues;
  request["python"] = python;
  request["pythonVenv"] = pythonVenv;
  this->pendingRequest = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";
  if (this->ready) {
    this->process->write(this->pendingRequest);
    this->pendingRequest.clear();
  }
}

void ComputeWorker::cancel()
{
  if (!this->busy) return;
  const auto canceledResult = this->resultPath;
  QFile cancelFile(canceledResult + ".cancel");
  if (!cancelFile.open(QIODevice::WriteOnly)) {
    emit diagnostic(tr("Could not request compute cancellation: %1").arg(cancelFile.errorString()));
    this->process->terminate();
    return;
  }
  QTimer::singleShot(1000, this, [this, canceledResult] {
    if (!this->busy || this->resultPath != canceledResult) return;
    this->process->terminate();
  });
}

void ComputeWorker::processMetadata()
{
  QFile parameters(this->resultPath + ".parameters.json");
  if (parameters.open(QIODevice::ReadOnly)) {
    emit parametersDiscovered(this->requestSource, QString::fromUtf8(parameters.readAll()));
  }
  QFile dependencies(this->resultPath + ".dependencies.json");
  if (dependencies.open(QIODevice::ReadOnly)) {
    QStringList paths;
    for (const auto& path : QJsonDocument::fromJson(dependencies.readAll()).array()) {
      paths.push_back(path.toString());
    }
    emit dependenciesDiscovered(this->requestSource, paths);
  }
}

void ComputeWorker::processOutput()
{
  while (this->process->canReadLine()) {
    const auto response = this->process->readLine().trimmed();
    if (response == "ready") {
      this->consecutiveFailures = 0;
      this->ready = true;
      if (!this->pendingRequest.isEmpty()) {
        this->process->write(this->pendingRequest);
        this->pendingRequest.clear();
      }
      continue;
    }
    if (response == "pong") continue;
    if (response.startsWith("progress\t")) {
      emit progress(response.mid(9).toInt());
    } else if (response == "done") {
      this->busy = false;
      this->request = Request::NONE;
      processMetadata();
      auto geometry = import_off(this->resultPath.toStdString(), Location::NONE);
      emit done(std::shared_ptr<const Geometry>(std::move(geometry)));
    } else if (response == "previewdone") {
      this->busy = false;
      this->request = Request::NONE;
      processMetadata();
      auto products = std::make_shared<CsgInfo>();
      if (!products->read_products((this->resultPath + ".products.json").toStdString())) {
        products.reset();
      }
      emit previewDone(std::move(products));
    } else if (response == "error" || response == "cancelled") {
      this->busy = false;
      const auto request = this->request;
      this->request = Request::NONE;
      if (request == Request::PREVIEW) emit previewDone({});
      else emit done({});
    }
  }
}
