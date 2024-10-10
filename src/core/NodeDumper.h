#pragma once

#include <sstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "core/NodeVisitor.h"
#include "core/node.h"
#include "core/NodeCache.h"

// GroupNodeChecker does a quick first pass to count children of group nodes
// If a GroupNode has 0 children, don't include in node id strings
// If a GroupNode has 1 child, we replace it with its child
// This makes id strings much more compact for deeply nested trees, recursive scad scripts,
// and increases likelihood of node cache hits.
class GroupNodeChecker : public NodeVisitor
{
public:
  GroupNodeChecker() = default;

  Response visit(State& state, const AbstractNode& node) override;
  Response visit(State& state, const GroupNode& node) override;
  void incChildCount(int groupNodeIndex);
  int getChildCount(int groupNodeIndex) const;
  void reset() { groupChildCounts.clear(); }

private:
  // stores <node_idx,nonEmptyChildCount> for each group node
  std::unordered_map<int, int> groupChildCounts;
};

class NodeDumper : public NodeVisitor
{
public:
  NodeDumper(NodeCache& cache, std::shared_ptr<const AbstractNode> root_node, std::string indent, bool idString) :
    cache(cache), indent(std::move(indent)), idString(idString), root(std::move(root_node)) {
    if (idString) {
      groupChecker.traverse(*root);
    }
  }

  Response visit(State& state, const AbstractNode& node) override;
  Response visit(State& state, const GroupNode& node) override;
  Response visit(State& state, const ListNode& node) override;
  Response visit(State& state, const RootNode& node) override;

private:
  void initCache();
  void finalizeCache();
  bool isCached(const AbstractNode& node) const;

  NodeCache& cache;
  // Output Formatting options
  std::string indent;
  bool idString;

  int currindent{0};
  std::shared_ptr<const AbstractNode> root;
  GroupNodeChecker groupChecker;
  std::ostringstream dumpstream;

};


