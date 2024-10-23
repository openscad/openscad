#pragma once

#include <cstddef>
#include <memory>
#include <vector>

class Context;

/*
 * Keeps track of the approximate number of Values stored as part of an
 * EvaluationSession, and in particular the number of objects that
 * the garbage collection algorithm needs to explore. This is used to keep
 * track of when a garbage collection run is due.
 *
 * Counts one point for each context, each context variable, and each element
 * in a VectorType value.
 */
class HeapSizeAccounting
{
public:
  void addContext(size_t number = 1) { count += number; }
  void removeContext(size_t number = 1) { count -= number; }
  void addContextVariable(size_t number = 1) { count += number; }
  void removeContextVariable(size_t number = 1) { count -= number; }
  void addVectorElement(size_t number = 1) { count += number; }
  void removeVectorElement(size_t number = 1) { count -= number; }

  [[nodiscard]] size_t size() const { return count; }

private:
  size_t count = 0;
};

class ContextMemoryManager
{
public:
  ~ContextMemoryManager();

  void addContext(const std::shared_ptr<Context>& context);
  void releaseContext() { heapSizeAccounting.removeContext(); }

  HeapSizeAccounting& accounting() { return heapSizeAccounting; }

private:
  std::vector<std::weak_ptr<Context>> managedContexts;
  HeapSizeAccounting heapSizeAccounting;
  size_t nextGarbageCollectSize = 0;
};
