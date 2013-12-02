#include "cgalworker.h"
#include <QThread>

#include "Tree.h"
#include "GeometryEvaluator.h"
#include "CGALEvaluator.h"
#include "progress.h"
#include "printutils.h"

CGALWorker::CGALWorker()
{
	this->thread = new QThread();
	if (this->thread->stackSize() < 1024*1024) this->thread->setStackSize(1024*1024);
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
	shared_ptr<const CGAL_Nef_polyhedron> root_N;
	try {
		GeometryEvaluator evaluator(*this->tree);
		root_N = evaluator.cgalevaluator->evaluateCGALMesh(*this->tree->root());
	}
	catch (const ProgressCancelException &e) {
		PRINT("Rendering cancelled.");
	}

	emit done(root_N);
	thread->quit();
}
