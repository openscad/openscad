#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/BaseVisitable.h"
#include "core/NodeCache.h"
#include "core/NodeVisitor.h"
#include "core/node.h"

class NodeDumper : public NodeVisitor
{
public:
  NodeDumper(NodeCache& cache, std::shared_ptr<const AbstractNode> root_node, std::string indent,
             bool idString)
    : cache(cache), indent(std::move(indent)), idString(idString), root(std::move(root_node))
  {
  }

  Response visit(State& state, const AbstractNode& node) override;
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
  std::ostringstream dumpstream;
};
