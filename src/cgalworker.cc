#include "cgalworker.h"
#include <QThread>

#include "Tree.h"
#include "CGALEvaluator.h"
#include "progress.h"
#include "printutils.h"

CGALWorker::CGALWorker()
{
	this->thread = new QThread();
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
	CGAL_Nef_polyhedron *root_N = NULL;
	try {
		CGALEvaluator evaluator(*this->tree);
		root_N = new CGAL_Nef_polyhedron(evaluator.evaluateCGALMesh(*this->tree->root()));
	}
	catch (const ProgressCancelException &e) {
		PRINT("Rendering cancelled.");
	}

	emit done(root_N);
	thread->quit();
}
