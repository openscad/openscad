#include "ThreadedNodeVisitor.h"

#include "printutils.h"
#include <thread>
#include <iostream>
#include <condition_variable>

using namespace std;

int ThreadedNodeVisitor::Parallelism = 0;

namespace {

constexpr bool THREAD_DEBUG = false;

class WorkItem {
public:
    WorkItem(int numChildren) : state(nullptr), pendingChildren(numChildren) {}
    State state;
    const AbstractNode *node = nullptr;
    std::atomic<int> pendingChildren;
    std::shared_ptr<WorkItem> parentWork;
};

class ProcessingContext {
public:
    ProcessingContext() {}
    std::queue<std::shared_ptr<WorkItem>> workQueue;
    // This lock is required when reading or writing the workQueue
    std::mutex queueMutex;
    // The condition variable is signaled whenever a new item is added to the queue.
    std::condition_variable cv;

    bool exitNow() {
        return _abort || _finished | _canceled;
    }

    void cancel() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            _canceled = true;
        }
        cv.notify_all();
    }

    bool isCanceled() const {
        return _canceled;
    }

    void abort() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            _abort = true;
        }
        cv.notify_all();
    }

    bool isAborted() const {
        return _abort;
    }

    void finish() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            _finished = true;
        }
        cv.notify_all();
    }

    void pushWorkItem(std::shared_ptr<WorkItem> item) {
        {
            std::lock_guard<std::mutex> lk(queueMutex);
            workQueue.push(std::move(item));
        }
        cv.notify_one();
    }

private:
    bool _abort = false;
    bool _finished = false;
    bool _canceled = false;
};

// This is the main function for each of the worker threads. It reads items from
// the work queue and processes them. It exits when the context signals to abort
// or that it is finished.
void ProcessWorkItems(ProcessingContext*ctx, NodeVisitor*visitor) {
    std::shared_ptr<WorkItem> nextWorkItem;

    while (!ctx->exitNow()) {
        std::shared_ptr<WorkItem> workItem;

        if (nextWorkItem) {
            workItem = nextWorkItem;
            nextWorkItem = nullptr;
        } else {
            // Wait for a work item to process
            std::unique_lock<std::mutex> lk(ctx->queueMutex);
            ctx->cv.wait(lk, [ctx]() {
                return !ctx->workQueue.empty() || ctx->exitNow();
            });

            if (ctx->exitNow()) {
                return;
            }

            if (ctx->workQueue.empty()) {
                continue;
            }

            // Get available work item
            workItem = ctx->workQueue.front();
            ctx->workQueue.pop();
            lk.unlock();
        }


        if (THREAD_DEBUG){
            // cout << "Processing item" << endl;
            cout << '*';
            fflush(stdout);
        }

        // Run postfix
        try {
            Response response = workItem->node->accept(workItem->state, *visitor);
            if (response == Response::AbortTraversal) {
                ctx->abort();
                return;
            }
        } catch (ProgressCancelException) {
            ctx->cancel();
            return;
        }

        if (workItem->parentWork) {
            // decrement remaining child count for pending parent work item. If this
            // was the last one, push the parent work item onto the queue.
            int x = workItem->parentWork->pendingChildren.fetch_sub(1);
            if (x == 1) {
                if (THREAD_DEBUG) {
                    // cout << "Pushing parent work item" << endl;
                    cout << '^';
                    fflush(stdout);
                }

                nextWorkItem = workItem->parentWork;
            }
        } else {
            // A parentless item is the root
            if (THREAD_DEBUG) {
                cout << "Finished traversing root item" << endl;
            }
            ctx->finish();
        }
    }
}

void _traverseThreadedRecursive(ProcessingContext*ctx,  NodeVisitor*visitor,
    std::shared_ptr<WorkItem> parentWorkItem, const AbstractNode &node, const class State &state) {

    if (ctx->exitNow()) return; // Abort immediately

    // Run prefix
    State newstate = state;
    newstate.setNumChildren(node.getChildren().size());
    newstate.setPrefix(true);
    newstate.setParent(state.parent());
    Response response;
    try {
        response = node.accept(newstate, *visitor);
    } catch (ProgressCancelException) {
        ctx->cancel();
        return;
    }

    if (response == Response::AbortTraversal) {
        ctx->abort();
        return;
    }

    // build postfix work item
    std::shared_ptr<WorkItem> postfixWorkItem = std::make_shared<WorkItem>(node.getChildren().size());
    postfixWorkItem->state = newstate;
    postfixWorkItem->state.setPrefix(false);
    postfixWorkItem->state.setPostfix(true);
    postfixWorkItem->node = &node;
    postfixWorkItem->parentWork = parentWorkItem;

    if (response == Response::PruneTraversal || node.getChildren().empty()) {
        // leaf node - queue parent work directly
        ctx->pushWorkItem(postfixWorkItem);
        return;
    }

    // Recurse
    newstate.setParent(&node);
    for(const auto &chnode : node.getChildren()) {
        _traverseThreadedRecursive(ctx, visitor, postfixWorkItem, *chnode, newstate);
        if (ctx->exitNow()) return; // Abort immediately
    }
}

} // namespace

// This begins by starting a fixed number of worker threads at the start of
// traversal and scheduling work amongst them as it becomes available. A
// condition variable is used to efficiently sleep threads while work is
// unavailable.
//
// The geometry tree is traversed top-to-bottom recursively, running prefix
// traversal for each of the nodes on the way down. This happens serially,
// ensuring that each node's prefix traversal happens before any of it's
// children's.
//
// The postfix traversals are usually (much) more expensive to run than the
// prefix traversals and these are what we run in parallel. The important
// constraint on running these in parallel is that a given node's postfix
// traversal can not be run until the postfix traversals for each of the child
// nodes is run.
//
// As the tree is recursively traversed top-to-bottom, a tree of pending postfix
// traversals is built. In order to keep track of which nodes are ready to run,
// each node keeps a counter of how many child nodes have yet to complete their
// postfix traversals. Each time a postfix traversal is completed, the pending
// child count of it's parent node is decremented. When the counter gets to
// zero, that node is pushed onto the work queue and is executed as soon as a
// thread is available.
Response ThreadedNodeVisitor::traverseThreaded(const AbstractNode &node, const class State &state) {
    // Create the context that will be passed to all recursive calls
    ProcessingContext ctx;

    // Start worker threads. Use Parallism if nonzero, otherwise use as many
    // threads as the system has cores.
    const size_t numThreads = Parallelism != 0 ?
                    Parallelism :
                    std::thread::hardware_concurrency();

    std::vector<std::thread> workers;
    for (int i = 0; i < numThreads; i++) {
        std::thread t([&ctx, this](){
            try {
                ProcessWorkItems(&ctx, this);
            } catch (...) {
                cerr << "UNHANDLED EXCEPTION IN WORKER THREAD" << endl;
                exit(1);
            }
        });
        workers.push_back(std::move(t));
    }
    PRINTB("Started %d worker threads", numThreads);

    // Recursively do all prefix traversals and schedule postfix traversals to
    // happen later.
    _traverseThreadedRecursive(&ctx, this, nullptr, node, state);

    // Wait for threads to finish processing all postfix traversals.
    for (auto& t : workers) t.join();

    if (THREAD_DEBUG) {
        cout << "JOINED THREADS" << endl;
    }

    // Re-throw exception if we ended early due to a user-requested cancel.
    if (ctx.isCanceled()) {
        throw ProgressCancelException();
    }

    return ctx.isAborted() ? Response::AbortTraversal : Response::ContinueTraversal;
}
