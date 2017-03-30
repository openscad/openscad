#include "ThreadedNodeVisitor.h"
#include "node.h"
#include "state.h"
#include "feature.h"
#include "printutils.h"
#include <algorithm>

#include <boost/thread.hpp>
#include <atomic>
#include <stack>
#include <map>
#include <string>

#include "Tree.h"
#include "progress.h"
#include "cgalutils.h"

const char *ResponseStr[3] = { "Continue", "Abort", "Prune" };

std::atomic<int> numRunning;

class TraverseRunner
{
public:
	TraverseData *data;
	boost::thread *thread;

	TraverseRunner(TraverseData *_data) : data(_data), thread(NULL) { }
	~TraverseRunner()
	{
		if (thread)
		{
			delete thread;
			thread = NULL;
		}
	}

	template <class ThreadFunc>
	void startRunner(ThreadFunc f)
	{
		thread = new boost::thread(f);
	}
};

class TraverseData
{
	enum TraverseDataState
	{
		NONE,
		PREFIXED,
		RUNNING,
		FINISHED
	};
	TraverseData *parent;
	std::string idString;
	const AbstractNode *node;
	State state;
	size_t depth;
	Response response;
	TraverseDataState dataState;
	std::list<TraverseData*> children;

public:
	TraverseData(const std::string &_idString, const AbstractNode *_node, const State &_state, size_t _depth)
		: parent(NULL)
		, idString(_idString)
		, node(_node)
		, state(_state)
		, depth(_depth)
		, response(ContinueTraversal)
		, dataState(NONE)
	{
	}

	~TraverseData()
	{
		for (auto child : children)
			delete child;
	}

	TraverseData *getParent() const { return parent; }
	const std::string &getIdString() const { return idString; }
	const AbstractNode *getNode() const { return node; }
	int getId() const { return node->index(); }
	size_t getDepth() const { return depth; }
	Response getResponse() const { return response; }

	void addChild(TraverseData *data)
	{
		data->parent = this;
		children.push_back(data);
	}

	TraverseRunner *getRunner(ThreadedNodeVisitor *traverser, bool &lastLeaf)
	{
		TraverseRunner *result = NULL;
		lastLeaf = false;
		if (dataState < RUNNING)
		{
			bool hasUnfinishedChild = false;
			for (auto child : children)
			{
				if (child->dataState != FINISHED)
					hasUnfinishedChild = true;
				bool dummy = false;
				result = child->getRunner(traverser, dummy);
				if (result != NULL)
					break;
			}
			if (!hasUnfinishedChild && result == NULL)
			{
				//PRINTB(" No unfinished children: %s", toString());
				if (!traverser->isRunning(this))
				{
					//PRINTB("++ Creating runner: %s", toString());
					dataState = RUNNING;
					result = new TraverseRunner(this);
					lastLeaf = true;
				}
			}
		}
		return result;
	}

	size_t countTotalLeaves() const
	{
		size_t result = 1;
		for (auto child : children)
			result += child->countTotalLeaves();
		return result;
	}

	size_t countLeafGeometries(const AbstractNode *node = NULL) const
	{
		auto kids = node == NULL ? this->node->getChildren() : node->getChildren();
		if (kids.empty())
			return 1;
		size_t result = 0;
		for (auto child : kids)
			result += countLeafGeometries(child);
		return result;
	}

	Response accept(bool postfix, BaseVisitor &visitor)
	{
		try
		{
			if (response != AbortTraversal)
			{
				state.setPrefix(!postfix);
				state.setPostfix(postfix);
				response = node->accept(state, visitor);
			}
		}
		catch (const ProgressCancelException &c)
		{
			PRINT("!!! Cancelling node traversal !!!");
			response = AbortTraversal;
		}
		catch (const std::exception &ex)
		{
			PRINTB("!!! Exception traversing node: %s", ex.what());
			response = AbortTraversal;
		}
		catch (...)
		{
			PRINT("!!! Unhandled exception traversing node");
			response = AbortTraversal;
		}
		if (postfix)
			dataState = FINISHED;
		else
			dataState = PREFIXED;
		return response;
	}

	std::string toNodeIdString() const
	{
		std::stringstream str;
		if (parent != NULL)
			str << parent->toNodeIdString() << ":";
		str << node->index();
		return str.str();
	}

	std::string toString() const
	{
		std::stringstream str;
		str << node->name() << " #" << toNodeIdString() << " at depth " << depth;
		return str.str();
	}
};

Response ThreadedNodeVisitor::traverseThreaded(const AbstractNode &node, const State &state /*= NodeVisitor::nullstate*/, TraverseData *parentData /*= NULL*/, size_t currentDepth /*= 0*/)
{
	if (!threaded)
		return traverse(node, state);

	Response response = ContinueTraversal;
	State newstate = state;
	newstate.setNumChildren(node.getChildren().size());

	std::string idString = tree.getIdString(node);
	TraverseData *nodeData = new TraverseData(idString, &node, newstate, currentDepth);
	if (parentData != NULL)
		parentData->addChild(nodeData);

	if (currentDepth == 0)
	{
		size_t geomCount = nodeData->countLeafGeometries();
		PRINTB("Threaded traversal phase 1: Generating %d leaf geometries", geomCount);
	}
		
	//PRINTB("  (%d) Running prefix", nodeData->getId());
	response = nodeData->accept(false, *this);
	//PRINTB("  (%d) Finished prefix: %s", nodeData->getId() % ResponseStr[nodeData->getResponse()]);
		
	// abort if the node aborted [prefix]
	if (response == AbortTraversal)
	{
		if (currentDepth == 0) // whoops!
			delete nodeData;
		return response;
	}

	const auto &kids = node.getChildren();
	// Pruned traversals mean don't traverse children
	if (response == ContinueTraversal && !kids.empty())
	{
		newstate.setParent(&node);
		for (const AbstractNode *chnode : kids) {
			response = traverseThreaded(*chnode, newstate, nodeData, currentDepth + 1);
			if (response == AbortTraversal)
				break;
		}
	}

	// abort if any child aborted [prefix]
	if (response == AbortTraversal)
	{
		if (currentDepth == 0) // whoops!
			delete nodeData;
		return response;
	}

	if (currentDepth == 0)
	{
		// multithreaded postfixing
		response = waitForIt(nodeData);
		//PRINT("DONE!!! Deleting the root traversal data");
		delete nodeData;
	}

	if (response != AbortTraversal) response = ContinueTraversal;
	return response;
}

// start runners using the available cores and wait for them to finish
Response ThreadedNodeVisitor::waitForIt(TraverseData *nodeData)
{
	Response response = ContinueTraversal;
	// lock CGAL errors on the main thread
	// this allows the spawned threads to catch their CGAL exceptions and not crash the whole app
	// Response::AbortTraversal is "bubbled-up" when it occurs
	CGALUtils::lockErrors(CGAL::THROW_EXCEPTION);
	size_t maxThreads = boost::thread::hardware_concurrency();
	size_t leafCount = nodeData->countTotalLeaves();
	PRINTB("Threaded traversal phase 2: Spawning %d threads on %d cores", leafCount % maxThreads);
	size_t leafCounter = 0;
	size_t totalJoinCount = 0;
	size_t runningCount = 0;
	bool lastLeaf = false;
	// wait loop
	while (true)
	{
		// step 1: start runners unless aborted or found the last leaf or enough are running
		if (response != AbortTraversal && !lastLeaf && maxThreads - runningCount > 0)
		{
			while (!lastLeaf && maxThreads - runningCount > 0)
			{
				TraverseRunner *runner = nodeData->getRunner(this, lastLeaf);
				if (runner == NULL)
				{
					assert(!lastLeaf && "Why doesn't the last leaf have data???");
					// we get here when a single parent is waiting on one or more children to complete
					break;
				}
				leafCounter++;
				if (lastLeaf)
				{
					//PRINTB("That's the last leaf (erm, the root)!!! counted=%d, count=%d", leafCounter % leafCount);
					assert(leafCount == leafCounter && "Why don't the leaf counts match???");
					assert(nodeData == runner->data && "Why isn't the last leaf its own data???");
				}
				auto f = [this, runner]
				{
					auto data = runner->data;
					auto running = std::atomic_fetch_add(&numRunning, 1);
					//PRINTB(">> Started thread %d (r=%d) %s", data->getId() % running % data->toString());
					//PRINTB("  (%d) Running postfix", data->getId());
					data->accept(true, *this);
					//PRINTB("  (%d) Finished postfix: %s", data->getId() % ResponseStr[data->getResponse()]);
					running = std::atomic_fetch_sub(&numRunning, 1);
					//PRINTB("<< Finished thread %d (r=%d) with result '%s' %s", data->getId() % running % ResponseStr[data->getResponse()] % data->toString());
				};
				// start the runner
				startRunner(f, runner);
				// increment the running thread count
				runningCount++;
			}
		}
		// if the running count is zero, no threads are running and no new threads were added = done!
		if (runningCount == 0)
		{
			//PRINT("No more pending children");
			assert(totalJoinCount == leafCounter && "Why weren't as many threads joined as started???");
			break;
		}
		// step 2: wait for any children to finish and do housekeeping
		//PRINT("Waiting for finished children");
		std::list<TraverseRunner*> finished;
		std::list<TraverseRunner*> stillRunning;
		waitForAny(finished, stillRunning);
		size_t joinCount = 0;
		for (auto runner : finished)
		{
			// check for abort
			if (runner->data->getResponse() == AbortTraversal)
				response = AbortTraversal;
			// release the memory
			delete runner;
			// decrement the running thread count, et.al.
			runningCount--;
			joinCount++;
			totalJoinCount++;
		}
		/*
		if (joinCount > 0 && runningCount != 0)
		{
			std::stringstream ss;
			bool first = true;
			for (auto runner : stillRunning)
			{
				if (!first)
					ss << ", ";
				ss << runner->data->getId();
				first = false;
			}
			PRINTB("Joined %d threads (%d/%d), still running: (%d) %s", joinCount % totalJoinCount % leafCount % runningCount % ss.str());
		}
		*/
	} // while(true) // wait loop

	CGALUtils::lockErrors(CGAL::THROW_EXCEPTION);
	return response;
}

// starts the runner with the given thread func
// called on the main thread
template <class ThreadFunc>
void ThreadedNodeVisitor::startRunner(ThreadFunc f, TraverseRunner *runner)
{
	// use a wrapper lambda function to additionally call finishRunner on the runner's thread
	auto wrapper = [this, f, runner]
	{
		f();
		finishRunner(runner);
	};

	// put the runner into the running map
	runner_lock.wait();
	running[runner->data->getIdString()] = runner;
	runner_lock.post();

	// now, start the thread
	runner->startRunner(wrapper);
}

// finishes runner: pulls it from running and moves it to finished
// called on the runner thread
void ThreadedNodeVisitor::finishRunner(TraverseRunner *runner)
{
	runner_lock.wait();
	// post ready_event if this is the first runner to finish
	if (finished.empty())
		ready_event.post();
	// move it to finished
	finished.push_back(runner);
	// pull it from running
	running.erase(runner->data->getIdString());
	runner_lock.post();
}

// waits for any runners to finish and fills finished with 'em
// fills stillRunning with the rest
// called on the main thread
void ThreadedNodeVisitor::waitForAny(std::list<TraverseRunner*> &finished, std::list<TraverseRunner*> &stillRunning)
{
	ready_event.wait();
	// ready_event was posted; fill the result lists
	runner_lock.wait();
	finished.assign(this->finished.begin(), this->finished.end());
	this->finished.clear();
	for (auto r : running)
		stillRunning.push_back(r.second);
	runner_lock.post();
}

// checks if the given data is running
// called on the main thread
bool ThreadedNodeVisitor::isRunning(TraverseData *data)
{
	const std::string &id = data->getIdString();
	runner_lock.wait();
	bool result = running.find(id) != running.end();
	runner_lock.post();
	return result;
}
