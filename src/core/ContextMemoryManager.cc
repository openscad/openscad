/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "core/ContextMemoryManager.h"

#include <variant>
#include <cassert>
#include <utility>
#include <memory>
#include <deque>
#include <map>
#include <unordered_set>
#include <vector>

#include "core/Context.h"
#include "core/Value.h"

/*
 * The garbage collector needs to know, for each Value, whether it stores
 * any references to other Values (VectorType, EmbeddedVectorType) or to
 * Contexts (FunctionType). It assumes that each Value that does this is
 * implemented as a shared_ptr to a data structure that stores the
 * Value or Context references without any further shared_ptr indirections.
 *
 * For each type of Value, we need to be able to determine:
 * - whether it's a type that can store Value or Context references, and if so:
 * - a unique identifier of the shared object making up this Value
 *   (shared_ptr.get() cast to a void pointer will do);
 * - the use_count() of the shared object;
 * - the list of values stored, and contexts referenced.
 */
using ValueIdentifier = void *;

struct IdentifierVisitor
{
  ValueIdentifier operator()(const UndefType&) const { return nullptr; }
  ValueIdentifier operator()(bool) const { return nullptr; }
  ValueIdentifier operator()(double) const { return nullptr; }
  ValueIdentifier operator()(const str_utf8_wrapper&) const { return nullptr; }
  ValueIdentifier operator()(const RangePtr&) const { return nullptr; }

  ValueIdentifier operator()(const VectorType& value) const { return value.ptr.get(); }
  ValueIdentifier operator()(const EmbeddedVectorType& value) const { return value.ptr.get(); }
  ValueIdentifier operator()(const ObjectType& value) const { return value.ptr.get(); }
  ValueIdentifier operator()(const FunctionPtr& value) const { return value.get().get(); }
};

struct UseCountVisitor
{
  int operator()(const UndefType&) const { return 0; }
  int operator()(bool) const { return 0; }
  int operator()(double) const { return 0; }
  int operator()(const str_utf8_wrapper&) const { return 0; }
  int operator()(const RangePtr&) const { return 0; }

  int operator()(const VectorType& value) const { return value.ptr.use_count(); }
  int operator()(const EmbeddedVectorType& value) const { return value.ptr.use_count(); }
  int operator()(const ObjectType& value) const { return value.ptr.use_count(); }
  int operator()(const FunctionPtr& value) const { return value.get().use_count(); }
};

struct EmbeddedValuesVisitor
{
  const std::vector<Value> *operator()(const UndefType&) const { return nullptr; }
  const std::vector<Value> *operator()(bool) const { return nullptr; }
  const std::vector<Value> *operator()(double) const { return nullptr; }
  const std::vector<Value> *operator()(const str_utf8_wrapper&) const { return nullptr; }
  const std::vector<Value> *operator()(const RangePtr&) const { return nullptr; }

  const std::vector<Value> *operator()(const VectorType& value) const { return &value.ptr->vec; }
  const std::vector<Value> *operator()(const EmbeddedVectorType& value) const { return &value.ptr->vec; }
  const std::vector<Value> *operator()(const ObjectType& value) const { return &value.ptr->values; }
  const std::vector<Value> *operator()(const FunctionPtr&) const { return nullptr; }
};

struct ReferencedContextVisitor
{
  const std::shared_ptr<const Context> *operator()(const UndefType&) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(bool) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(double) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(const str_utf8_wrapper&) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(const RangePtr&) const { return nullptr; }

  const std::shared_ptr<const Context> *operator()(const VectorType&) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(const EmbeddedVectorType&) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(const ObjectType&) const { return nullptr; }
  const std::shared_ptr<const Context> *operator()(const FunctionPtr& value) const { return &value->getContext(); }
};



/*
 * Finds all contexts that have an inbound reference from something that is not
 * another context.
 *
 * The algorithm tracks, for each reference-counted value and each context, the
 * number of inbound references that originate directly or indirectly from a
 * context variable. If this is equal to the number of total references to the
 * value/context, it is reachable only from a context. If a Value is reachable
 * only from a context, then any outgoing references from that Value are also
 * considered reachable-only-from-context, recursively.
 *
 * Implemented as a breadth first search to save on stack space.
 */
static std::vector<Context *> findRootContexts(const std::vector<std::shared_ptr<Context>>& managedContexts)
{
  std::map<ValueIdentifier, int> accountedValueReferences;
  std::map<const Context *, int> accountedContextReferences;
  std::unordered_set<const Context *> fullyAccountedContexts;

  std::deque<const Value *> valueQueue;
  std::deque<const std::shared_ptr<const Context> *> contextQueue;

  auto visitValue = [&](const Value& value) {
      ValueIdentifier identifier = std::visit(IdentifierVisitor(), value.getVariant());
      if (!identifier) {
        return;
      }

      if (!accountedValueReferences.count(identifier)) {
        accountedValueReferences[identifier] = 0;
      }
      int accountedReferences = ++accountedValueReferences[identifier];
      int requiredReferences = std::visit(UseCountVisitor(), value.getVariant());
      assert(accountedReferences <= requiredReferences);
      if (accountedReferences == requiredReferences) {
        const std::vector<Value> *embeddedValues = std::visit(EmbeddedValuesVisitor(), value.getVariant());
        if (embeddedValues) {
          for (const Value& embeddedValue : *embeddedValues) {
            valueQueue.push_back(&embeddedValue);
          }
        }

        const std::shared_ptr<const Context> *referencedContext = std::visit(ReferencedContextVisitor(), value.getVariant());
        if (referencedContext) {
          contextQueue.push_back(referencedContext);
        }
      }
    };
  auto visitContext = [&](const std::shared_ptr<const Context>& context) {
      if (!accountedContextReferences.count(context.get())) {
        accountedContextReferences[context.get()] = 0;
      }
      int accountedReferences = ++accountedContextReferences[context.get()];
      int requiredReferences = context.use_count();
      assert(accountedReferences <= requiredReferences);
      if (accountedReferences == requiredReferences) {
        fullyAccountedContexts.insert(context.get());
      }
    };

  for (const std::shared_ptr<Context>& context : managedContexts) {
    std::vector<const Value *> values = context->list_embedded_values();
    valueQueue.insert(valueQueue.end(), values.begin(), values.end());

    std::vector<const std::shared_ptr<const Context> *> referencedContexts = context->list_referenced_contexts();
    contextQueue.insert(contextQueue.end(), referencedContexts.begin(), referencedContexts.end());

    accountedContextReferences[context.get()] = 1;
  }

  while (!valueQueue.empty() || !contextQueue.empty()) {
    if (!valueQueue.empty()) {
      const Value *value = valueQueue.front();
      valueQueue.pop_front();
      visitValue(*value);
    } else {
      assert(!contextQueue.empty());
      const std::shared_ptr<const Context> *context = contextQueue.front();
      contextQueue.pop_front();
      visitContext(*context);
    }
  }

  std::vector<Context *> rootContexts;
  for (const std::shared_ptr<Context>& context : managedContexts) {
    if (!fullyAccountedContexts.count(context.get())) {
      rootContexts.push_back(context.get());
    }
  }
  return rootContexts;
}



/*
 * Finds all contexts reachable from a set of root contexts.
 *
 * Implemented as a breadth first search to save on stack space.
 */
static std::unordered_set<const Context *> findReachableContexts(const std::vector<Context *>& rootContexts)
{
  std::unordered_set<ValueIdentifier> valuesSeen;
  std::unordered_set<const Context *> contextsSeen;

  std::deque<const Value *> valueQueue;
  std::deque<const Context *> contextQueue;

  auto visitValue = [&](const Value& value) {
      ValueIdentifier identifier = std::visit(IdentifierVisitor(), value.getVariant());
      if (!identifier) {
        return;
      }
      if (!valuesSeen.count(identifier)) {
        valuesSeen.insert(identifier);
        valueQueue.push_back(&value);
      }
    };
  auto visitContext = [&](const Context *context) {
      if (!contextsSeen.count(context)) {
        contextsSeen.insert(context);
        contextQueue.push_back(context);
      }
    };

  contextsSeen.insert(rootContexts.begin(), rootContexts.end());
  contextQueue.insert(contextQueue.end(), rootContexts.begin(), rootContexts.end());
  while (!valueQueue.empty() || !contextQueue.empty()) {
    if (!valueQueue.empty()) {
      const Value *value = valueQueue.front();
      valueQueue.pop_front();

      const std::vector<Value> *embeddedValues = std::visit(EmbeddedValuesVisitor(), value->getVariant());
      if (embeddedValues) {
        for (const Value& embeddedValue : *embeddedValues) {
          visitValue(embeddedValue);
        }
      }

      const std::shared_ptr<const Context> *referencedContext = std::visit(ReferencedContextVisitor(), value->getVariant());
      if (referencedContext) {
        visitContext(referencedContext->get());
      }
    } else {
      assert(!contextQueue.empty());
      const Context *context = contextQueue.front();
      contextQueue.pop_front();

      std::vector<const Value *> values = context->list_embedded_values();
      for (const Value *value : values) {
        visitValue(*value);
      }

      std::vector<const std::shared_ptr<const Context> *> referencedContexts = context->list_referenced_contexts();
      for (const std::shared_ptr<const Context> *referencedContext : referencedContexts) {
        visitContext(referencedContext->get());
      }
    }
  }

  return contextsSeen;
}



/*
 * Clean up all unreachable contexts.
 */
static void collectGarbage(std::vector<std::weak_ptr<Context>>& managedContexts)
{
  /*
   * Garbage collection consists of three phases.
   *
   * In phase 1, we count all references from a context to another context.
   * If the number of references to a context from other contexts is equal
   * to the total number of references to that context, that means the
   * context is only reachable from other contexts. If not, that means the
   * context is reachable from somewhere in the evaluation execution stack.
   *
   * In phase 2, we compute forwards reachability of contexts, starting from
   * those contexts that are reachable from outside the context system. We
   * mark all contexts reachable from there.
   *
   * In phase 3, we delete all contexts that are not marked as reachable.
   * We do this by deleting all values held in those contexts, which breaks
   * any reference cycles to them.
   */

  /*
   * Lock all contexts to prevent deletion during reachability analysis.
   */
  std::vector<std::shared_ptr<Context>> allContexts;
  for (const std::weak_ptr<Context>& managedContext : managedContexts) {
    std::shared_ptr<Context> context = managedContext.lock();
    if (context) {
      allContexts.push_back(std::move(context));
    }
  }

  std::vector<Context *> rootContexts = findRootContexts(allContexts);

  std::unordered_set<const Context *> reachableContexts = findReachableContexts(rootContexts);

#ifdef DEBUG
  std::vector<std::weak_ptr<Context>> removedContexts;
#endif

  managedContexts.clear();
  for (std::shared_ptr<Context>& context : allContexts) {
    if (reachableContexts.count(context.get())) {
      managedContexts.emplace_back(context);
    } else {
      context->clear();
#ifdef DEBUG
      removedContexts.emplace_back(context);
#endif
    }
  }

  /*
   * At this point, the unreachable contexts have only acyclic references
   * to each other, as well as one reference from allContexts. Once
   * allContexts gets emptied, there will be no references holding the whole
   * edifice up.
   *
   * To avoid a cascading context destructor chain that might overflow the
   * stack, clear out allContexts back-to-front, which is reverse creation
   * order.
   */
  while (!allContexts.empty()) {
    allContexts.pop_back();
  }

#ifdef DEBUG
  for (const auto& context : removedContexts) {
    assert(context.expired());
  }
#endif
}



ContextMemoryManager::~ContextMemoryManager()
{
  collectGarbage(managedContexts);
  assert(managedContexts.empty());
  assert(heapSizeAccounting.size() == 0);
}

void ContextMemoryManager::addContext(const std::shared_ptr<Context>& context)
{
  heapSizeAccounting.addContext();
  context->setAccountingAdded();   // avoiding bad accounting when an exception threw in constructor issue #3871

  /*
   * If we are holding the last copy to this context, no point in invoking
   * the garbage collection machinery, we can just let context get destroyed
   * right away.
   */
  if (context.use_count() > 1) {
    managedContexts.emplace_back(context);

    if (heapSizeAccounting.size() >= nextGarbageCollectSize) {
      collectGarbage(managedContexts);
      /*
       * The cost of a garbage collection run is proportional to the heap
       * size. By scheduling the next run at twice the *remaining* heap size,
       * the total processing time of garbage collection throughout an
       * evaluation session is at most proportional to the total heap size
       * accumulated during the session, while keeping the maximum memory
       * used at any point at most twice the amount of necessary memory usage
       * (i.e. waste is at most a factor 2 overhead).
       */
      nextGarbageCollectSize = heapSizeAccounting.size() * 2;
    }
  }
}
