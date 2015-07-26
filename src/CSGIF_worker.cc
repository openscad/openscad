#include "CSGIF_worker.h"
#include <QThread>

#include "Tree.h"
#include "GeometryEvaluator.h"
#include "progress.h"
#include "printutils.h"

CSGIF_Worker::CSGIF_Worker()
{
	this->thread = new QThread();
	if (this->thread->stackSize() < 1024*1024) this->thread->setStackSize(1024*1024);
	connect(this->thread, SIGNAL(started()), this, SLOT(work()));
	moveToThread(this->thread);
}

CSGIF_Worker::~CSGIF_Worker()
{
	delete this->thread;
}

void CSGIF_Worker::start(const Tree &tree)
{
	this->tree = &tree;
	this->thread->start();
}

void CSGIF_Worker::work()
{
	shared_ptr<const Geometry> root_geom;
	try {
		GeometryEvaluator evaluator(*this->tree);
		root_geom = evaluator.evaluateGeometry(*this->tree->root(), true);
	}
	catch (const ProgressCancelException &e) {
		PRINT("Rendering cancelled.");
	}

	emit done(root_geom);
	thread->quit();
}
