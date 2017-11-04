#include "cgalworker.h"
#include <QThread>

#include "Tree.h"
#include "GeometryEvaluator.h"
#include "progress.h"
#include "printutils.h"

CGALWorker::CGALWorker()
{
	this->thread = new QThread();
	if (this->thread->stackSize() < 1024 * 1024) this->thread->setStackSize(1024 * 1024);
	connect(this->thread, SIGNAL(started()), this, SLOT(work()));
	moveToThread(this->thread);
}

CGALWorker::~CGALWorker()
{
	delete this->thread;
}

void CGALWorker::start(const Tree &tree)
{
	this->tree = &tree;
	this->thread->start();
}

void CGALWorker::work()
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
