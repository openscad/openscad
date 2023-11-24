#include "CGALWorker.h"
#include <QThread>
#ifdef __APPLE__
#include <pthread/qos.h>
#endif

#include "Tree.h"
#include "GeometryEvaluator.h"
#include "progress.h"
#include "printutils.h"
#include "exceptions.h"

CGALWorker::CGALWorker()
{
  this->tree = nullptr;
  this->thread = new QThread();
  this->thread->setPriority(QThread::HighPriority);
  if (this->thread->stackSize() < 1024 * 1024) this->thread->setStackSize(1024 * 1024);
  connect(this->thread, SIGNAL(started()), this, SLOT(work()));
  moveToThread(this->thread);
}

CGALWorker::~CGALWorker()
{
  delete this->thread;
}

void CGALWorker::start(const Tree& tree)
{
  this->tree = &tree;
  this->thread->start();
}

void CGALWorker::work()
{
#ifdef __APPLE__
  pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
  // this is a worker thread: we don't want any exceptions escaping and crashing the app.
  shared_ptr<const Geometry> root_geom;
  try {
    GeometryEvaluator evaluator(*this->tree);
    root_geom = evaluator.evaluateGeometry(*this->tree->root(), true);
  } catch (const ProgressCancelException& e) {
    LOG("Rendering cancelled.");
  } catch (const HardWarningException& e) {
    LOG("Rendering cancelled on first warning.");
  } catch (const std::exception& e) {
    LOG(message_group::Error, "Rendering cancelled by exception %1$s", e.what());
  } catch (...) {
    LOG(message_group::Error, "Rendering cancelled by unknown exception.");
  }

  emit done(root_geom);
  thread->quit();
}
