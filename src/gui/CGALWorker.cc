#include "gui/CGALWorker.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#include <filesystem>
#include <memory>

#include "core/AST.h"
#include "geometry/PolySet.h"
#include "io/import.h"
#include "utils/printutils.h"

CGALWorker::CGALWorker()
{
  this->sourceFile = nullptr;
  this->process = new QProcess();
  connect(this->process, &QProcess::readyReadStandardOutput, this, &CGALWorker::processOutput);
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
            const auto interrupted = this->busy;
            const auto request = this->request;
            this->busy = false;
            this->request = Request::NONE;
            startProcess();
            if (interrupted && request == Request::PREVIEW) emit previewDone({});
            else if (interrupted) emit done({});
          });
  startProcess();
}

void CGALWorker::startProcess()
{
  this->process->start(QCoreApplication::applicationFilePath(), {"--compute-worker"});
  if (!this->process->waitForStarted()) {
    LOG(message_group::Error, "Could not start compute worker: %1$s",
        this->process->errorString().toStdString());
  }
}

CGALWorker::~CGALWorker()
{
  this->stopping = true;
  this->process->write("quit\n");
  if (!this->process->waitForFinished(1000)) {
    this->process->kill();
    this->process->waitForFinished();
  }
  delete this->process;
  delete this->sourceFile;
  if (!this->resultPath.isEmpty()) QFile::remove(this->resultPath);
}

qint64 CGALWorker::processId() const
{
  return this->process->processId();
}

void CGALWorker::start(const QString& source, const QString& filename)
{
  startRequest("render", ".off", source, filename);
}

void CGALWorker::startPreview(const QString& source, const QString& filename)
{
  startRequest("preview", ".csg", source, filename);
}

void CGALWorker::startRequest(const QString& command, const QString& suffix, const QString& source,
                              const QString& filename)
{
  delete this->sourceFile;
  if (!this->resultPath.isEmpty()) QFile::remove(this->resultPath);

  const auto directory = filename.isEmpty() ? QDir::tempPath() : QFileInfo(filename).absolutePath();
  this->displayFilename = filename.isEmpty() ? QString("Untitled.scad") : filename;
  this->sourceFile = new QTemporaryFile(directory + "/.openscad-worker-XXXXXX.scad");
  if (!this->sourceFile->open()) {
    LOG(message_group::Error, "Could not create compute worker input: %1$s",
        this->sourceFile->errorString().toStdString());
    emit done({});
    return;
  }
  this->sourceFile->write(source.toUtf8());
  this->sourceFile->flush();
  this->resultPath = this->sourceFile->fileName() + suffix;
  this->request = command == "preview" ? Request::PREVIEW : Request::RENDER;
  this->busy = true;
  this->process->write(
    QString("%1\t%2\t%3\n").arg(command, this->sourceFile->fileName(), this->resultPath).toUtf8());
}

void CGALWorker::cancel()
{
  const auto request = this->request;
  this->stopping = true;
  if (this->process->state() != QProcess::NotRunning) {
    this->process->kill();
    this->process->waitForFinished();
  }
  this->busy = false;
  this->request = Request::NONE;
  startProcess();
  this->stopping = false;
  if (request == Request::PREVIEW) emit previewDone({});
  else emit done({});
}

void CGALWorker::processOutput()
{
  while (this->process->canReadLine()) {
    const auto response = this->process->readLine().trimmed();
    if (response == "ready" || response == "pong") continue;
    if (response == "done") {
      this->busy = false;
      this->request = Request::NONE;
      auto geometry = import_off(this->resultPath.toStdString(), Location::NONE);
      emit done(std::shared_ptr<const Geometry>(std::move(geometry)));
    } else if (response == "previewdone") {
      this->busy = false;
      this->request = Request::NONE;
      QFile file(this->resultPath);
      emit previewDone(file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString{});
    } else if (response == "error") {
      this->busy = false;
      const auto request = this->request;
      this->request = Request::NONE;
      if (request == Request::PREVIEW) emit previewDone({});
      else emit done({});
    }
  }
}
