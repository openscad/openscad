#include "cgalworker.h"
#include <QThread>

#include "Tree.h"
#include "GeometryEvaluator.h"
#include "progress.h"
#include "printutils.h"
#include "exceptions.h"
#include "pcache.h"
#include "PCSettings.h"

CGALWorker::CGALWorker()
{
	this->tree = nullptr;
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
#ifdef ENABLE_HIREDIS
#if BOOST_VERSION > 105800
    if(PCSettings::instance()->enablePersistentCache){
        PCache::getInst()->init(PCSettings::instance()->ipAddress, PCSettings::instance()->port, PCSettings::instance()->password);
        if(PCSettings::instance()->enableAuth){
            PCache::getInst()->connectWithPassword();
        }else{
            PCache::getInst()->connect();
        }
    }
#endif
#endif
	shared_ptr<const Geometry> root_geom;
	try {
		GeometryEvaluator evaluator(*this->tree);
		root_geom = evaluator.evaluateGeometry(*this->tree->root(), true);
	}
	catch (const ProgressCancelException &e) {
		LOG(message_group::None,Location::NONE,"","Rendering cancelled.");
	}
	catch (const HardWarningException &e) {
		LOG(message_group::None,Location::NONE,"","Rendering cancelled on first warning.");
	}
#ifdef ENABLE_HIREDIS
#if BOOST_VERSION > 105800
    PCache::getInst()->disconnect();
#endif
#endif

	emit done(root_geom);
	thread->quit();
}
