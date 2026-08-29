#include "gui/GeometryWorker.h"

#include <QThread>
#include <exception>
#include <memory>

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include "core/Tree.h"
#include "core/progress.h"
#include "geometry/GeometryEvaluator.h"
#include "utils/exceptions.h"
#include "utils/printutils.h"

#ifdef ENABLE_PYTHON
#include "python/python_public.h"
#endif

GeometryWorker::GeometryWorker()
{
  this->thread = new QThread();
  if (this->thread->stackSize() < 1024 * 1024) this->thread->setStackSize(1024 * 1024);
  connect(this->thread, &QThread::started, this, &GeometryWorker::work);
  moveToThread(this->thread);
}

GeometryWorker::~GeometryWorker()
{
  this->thread->quit();
  this->thread->wait();
  delete this->thread;
}

void GeometryWorker::start(const Tree& tree)
{
#ifdef ENABLE_PYTHON
  python_unlock();
#endif
  this->tree = &tree;
  this->thread->start();
}

void GeometryWorker::work()
{
#ifdef ENABLE_PYTHON
  python_lock();
#endif
  std::shared_ptr<const Geometry> root_geom;
  try {
    GeometryEvaluator evaluator(*this->tree);
    root_geom = evaluator.evaluateGeometry(*this->tree->root(), true);
#ifdef ENABLE_MANIFOLD
    if (auto manifold = std::dynamic_pointer_cast<const ManifoldGeometry>(root_geom)) {
      if (manifold->getManifold().Status() != manifold::Manifold::Error::NoError)
        LOG(message_group::Error, "Rendering cancelled due to unknown manifold error.");
    }
#endif
  } catch (const ProgressCancelException&) {
    LOG("Rendering cancelled.");
  } catch (const HardWarningException&) {
    LOG("Rendering cancelled on first warning.");
  } catch (const std::exception& e) {
    LOG(message_group::Error, "Rendering cancelled by exception %1$s", e.what());
  } catch (...) {
    LOG(message_group::Error, "Rendering cancelled by unknown exception.");
  }
#ifdef ENABLE_PYTHON
  python_unlock();
#endif
  emit done(root_geom);
  thread->quit();
}
