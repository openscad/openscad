#include "ThreadedNodeVisitor.h"

#include <thread>
#include <iostream>
#include <condition_variable>

using namespace std;

namespace {

constexpr bool THREAD_DEBUG = true;

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
    ProcessingContext() : abort(false), finished(false) {}
    std::queue<std::shared_ptr<WorkItem>> workQueue;
    // This lock is required when reading or writing the workQueue
    std::mutex queueMutex;
    // The condition variable is signaled whenever a new item is added to the queue.
    std::condition_variable cv;

    std::atomic<bool> abort; // Threads check this to see if they need to abort
    std::atomic<bool> finished;

    void pushWorkItem(std::shared_ptr<WorkItem> item) {
        {
            std::lock_guard<std::mutex> lk(queueMutex);
            workQueue.push(std::move(item));
        }
        cv.notify_one();
    }
};

// This is the main function for each of the worker threads. It reads items from
// the work queue and processes them. It exits when the context signals to abort
// or that it is finished.
void ProcessWorkItems(ProcessingContext*ctx, NodeVisitor*visitor) {
    std::shared_ptr<WorkItem> nextWorkItem;

    while (!ctx->abort && !ctx->finished) {
        std::shared_ptr<WorkItem> workItem;

        if (nextWorkItem) {
            workItem = nextWorkItem;
            nextWorkItem = nullptr;
        } else {
            // Wait for a work item to process
            std::unique_lock<std::mutex> lk(ctx->queueMutex);
            ctx->cv.wait(lk, [ctx]() {
                return !ctx->workQueue.empty() || ctx->abort || ctx->finished;
            });

            if (ctx->abort || ctx->finished) {
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
        Response response = workItem->node->accept(workItem->state, *visitor);
        if (response == Response::AbortTraversal) {
            ctx->abort = true;
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
            ctx->finished = true;
            // wake up each thread so it can see we're done and exit
            ctx->cv.notify_all();
        }
    }
}

void _traverseThreadedRecursive(ProcessingContext*ctx,  NodeVisitor*visitor,
    std::shared_ptr<WorkItem> parentWorkItem, const AbstractNode &node, const class State &state) {

    if (ctx->abort) return; // Abort immediately

    // Run prefix
    State newstate = state;
    newstate.setNumChildren(node.getChildren().size());
    newstate.setPrefix(true);
    newstate.setParent(state.parent());
    Response response = node.accept(newstate, *visitor);

    if (response == Response::AbortTraversal) {
        ctx->abort = true;
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
        if (ctx->abort) return; // Abort immediately
    }
}

} // namespace

Response ThreadedNodeVisitor::traverseThreaded(const AbstractNode &node, const class State &state) {
    // Create the context that will be passed to all recursive calls
    ProcessingContext ctx;

    // Start worker threads
    const size_t maxThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> workers;
    for (int i = 0; i < maxThreads; i++) {
        std::thread t([&ctx, this](){ProcessWorkItems(&ctx, this);});
        workers.push_back(std::move(t));
    }
    cout << "Started " << maxThreads << " worker threads" << endl;

    // Recursively do all prefix traversals and schedule postfix traversals to
    // happen later.
    _traverseThreadedRecursive(&ctx, this, nullptr, node, state);

    // Wait for threads to finish processing all postfix traversals.
    for (auto& t : workers) t.join();

    return ctx.abort ? Response::AbortTraversal : Response::ContinueTraversal;
}
