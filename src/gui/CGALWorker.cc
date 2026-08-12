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
  this->process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
  connect(this->process, &QProcess::readyReadStandardOutput, this, &CGALWorker::processOutput);
  connect(this->process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [this](int, QProcess::ExitStatus) {
            if (this->stopping) return;
            const auto interrupted = this->busy;
            this->busy = false;
            startProcess();
            if (interrupted) emit done({});
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
  delete this->sourceFile;
  if (!this->resultPath.isEmpty()) QFile::remove(this->resultPath);

  const auto directory = filename.isEmpty() ? QDir::tempPath() : QFileInfo(filename).absolutePath();
  this->sourceFile = new QTemporaryFile(directory + "/.openscad-worker-XXXXXX.scad");
  if (!this->sourceFile->open()) {
    LOG(message_group::Error, "Could not create compute worker input: %1$s",
        this->sourceFile->errorString().toStdString());
    emit done({});
    return;
  }
  this->sourceFile->write(source.toUtf8());
  this->sourceFile->flush();
  this->resultPath = this->sourceFile->fileName() + ".off";
  this->busy = true;
  this->process->write(
    QString("render\t%1\t%2\n").arg(this->sourceFile->fileName(), this->resultPath).toUtf8());
}

void CGALWorker::cancel()
{
  this->stopping = true;
  if (this->process->state() != QProcess::NotRunning) {
    this->process->kill();
    this->process->waitForFinished();
  }
  this->busy = false;
  startProcess();
  this->stopping = false;
  emit done({});
}

void CGALWorker::processOutput()
{
  while (this->process->canReadLine()) {
    const auto response = this->process->readLine().trimmed();
    if (response == "ready" || response == "pong") continue;
    if (response == "done") {
      this->busy = false;
      auto geometry = import_off(this->resultPath.toStdString(), Location::NONE);
      emit done(std::shared_ptr<const Geometry>(std::move(geometry)));
    } else if (response == "error") {
      this->busy = false;
      emit done({});
    }
  }
}
