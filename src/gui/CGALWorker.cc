#include "gui/CGALWorker.h"
#include <exception>
#include <memory>
#include <QThread>

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include "core/Tree.h"
#include "geometry/GeometryEvaluator.h"
#include "core/progress.h"
#include "utils/printutils.h"
#include "utils/exceptions.h"

CGALWorker::CGALWorker()
{
  this->tree = nullptr;
  this->thread = new QThread();
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
  // this is a worker thread: we don't want any exceptions escaping and crashing the app.
  std::shared_ptr<const Geometry> root_geom;
  try {
    GeometryEvaluator evaluator(*this->tree);
    root_geom = evaluator.evaluateGeometry(*this->tree->root(), true);

#ifdef ENABLE_MANIFOLD
    if (auto manifold = std::dynamic_pointer_cast<const ManifoldGeometry>(root_geom)) {
      // calling status forces evaluation
      // we should complete evaluation within the worker thread, so computation
      // will not block the GUI.
      if (manifold->getManifold().Status() != manifold::Manifold::Error::NoError)
        LOG(message_group::Error, "Rendering cancelled due to unknown manifold error.");
    }
#endif
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
