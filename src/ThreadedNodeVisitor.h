#pragma once

#include <stdint.h>
#include <map>
#include <list>
#include <stack>
#include "NodeVisitor.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"

// MinGW defines sprintf to libintl_sprintf which breaks usage of the
// Qt sprintf in QString. This is skipped if sprintf and _GL_STDIO_H
// is already defined, so the workaround defines sprintf as itself.
#ifdef __MINGW32__
#define _GL_STDIO_H
#define sprintf sprintf
#endif
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <memory>
#include <atomic>

// forward declaration: node specific data for threaded traversal
class TraverseData;
// forward declaration: encapsulates a "runner" for TraverseData objects
class TraverseRunner;
// forward declaration: custom cache to ensure geometries aren't deleted prematurely
class TraverseCache;

class ThreadedNodeVisitor : public NodeVisitor
{
	bool threaded;												// indicates this visitor should actually use threads
	std::atomic_flag runner_lock;								// locks access to the runners
	boost::interprocess::interprocess_semaphore ready_event;	// set when the first runner has finished
	std::list<TraverseRunner*> finished;						// a list of finished runners
	std::map<std::string, TraverseRunner*> running;				// the current runners indexed by TraverseData::idString
	TraverseCache *cache;										// custom cache to ensure geometries aren't deleted prematurely

	const Tree &tree;

	// starts the runner with the given thread func
	// called on the main thread
	template <class ThreadFunc> 
	void startRunner(ThreadFunc f, TraverseRunner *runner);

	// finishes runner: pulls it from running and moves it to finished
	// called on the runner thread
	void finishRunner(TraverseRunner *runner);

	// waits for any runners to finish and fills finished with 'em
	// fills stillRunning with the rest
	// called on the main thread
	void waitForAny(std::list<TraverseRunner*> &finished, std::list<TraverseRunner*> &stillRunning);

	// start runners using the available cores and wait for them to finish
	Response waitForIt(TraverseData *nodeData);

protected:
	virtual void smartCacheInsert(const AbstractNode &node, const shared_ptr<const Geometry> &geom);

public:
  ThreadedNodeVisitor(const Tree &_tree, bool _threaded = false)
	  : threaded(_threaded), runner_lock(ATOMIC_FLAG_INIT), ready_event(0), cache(NULL), tree(_tree) {
  }
  virtual ~ThreadedNodeVisitor() { }

  Response traverseThreaded(const AbstractNode &node, const class State &state = NodeVisitor::nullstate, TraverseData *parentData = NULL, size_t currentDepth = 0);

  // checks if the given data is running
  // called on the main thread
  bool isRunning(TraverseData *data);

  const Tree &getTree() const { return tree; }
};
