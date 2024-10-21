#pragma once

#include "core/NodeCache.h"
#include <tuple>
#include <memory>
#include <map>
#include <string>
#include <utility>

/*!
   For now, just an abstraction of the node tree which keeps a dump
   cache based on node indices around.

   Note that since node trees don't survive a recompilation, the tree cannot either.
 */
class Tree
{
public:
  Tree(std::shared_ptr<const AbstractNode> root = nullptr, std::string path = {}) : root_node(std::move(root)), document_path(std::move(path)) {}
  ~Tree();

  void setRoot(const std::shared_ptr<const AbstractNode>& root);
  void setDocumentPath(const std::string& path);
  const std::shared_ptr<const AbstractNode>& root() const { return this->root_node; }

  const std::string getString(const AbstractNode& node, const std::string& indent) const;
  const std::string getIdString(const AbstractNode& node) const;
  const std::string getDocumentPath() const;

private:
  std::shared_ptr<const AbstractNode> root_node;
  // keep a separate nodecache per tuple of NodeDumper constructor parameters
  mutable std::map<std::tuple<std::string, bool>, NodeCache> nodecachemap;
  std::string document_path;
};
