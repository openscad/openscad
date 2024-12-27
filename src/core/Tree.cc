#include "core/Tree.h"
#include "core/NodeDumper.h"

#include <memory>
#include <cassert>
#include <string>
#include <tuple>

Tree::~Tree()
{
  this->nodecachemap.clear();
}

/*!
   Returns the cached string representation of the subtree rooted by \a node.
   If node is not cached, the cache will be rebuilt.
 */
const std::string Tree::getString(const AbstractNode& node, const std::string& indent) const
{
  assert(this->root_node);
  bool idString = false;

  // Retrieve a nodecache given a tuple of NodeDumper constructor options
  NodeCache& nodecache = this->nodecachemap[std::make_tuple(indent, idString)];

  if (!nodecache.contains(node)) {
    NodeDumper dumper(nodecache, this->root_node, indent, idString);
    dumper.traverse(*this->root_node);
    assert(nodecache.contains(*this->root_node) &&
           "NodeDumper failed to create a cache");
  }
  return nodecache[node];
}

/*!
   Returns the cached ID string representation of the subtree rooted by \a node.
   If node is not cached, the cache will be rebuilt.

   The difference between this method and getString() is that the ID string
   is stripped for whitespace. Especially indentation whitespace is important to
   strip to enable cache hits for equivalent nodes from different scopes.
 */
const std::string Tree::getIdString(const AbstractNode& node) const
{
  assert(this->root_node);
  const std::string indent = "";
  const bool idString = true;

  // Retrieve a nodecache given a tuple of NodeDumper constructor options
  NodeCache& nodecache = this->nodecachemap[make_tuple(indent, idString)];

  if (!nodecache.contains(node)) {
    nodecache.clear();
    NodeDumper dumper(nodecache, this->root_node, indent, idString);
    dumper.traverse(*this->root_node);
    assert(nodecache.contains(*this->root_node) &&
           "NodeDumper failed to create id cache");
  }
  return nodecache[node];
}

/*!
   Sets a new root. Will clear the existing cache.
 */
void Tree::setRoot(const std::shared_ptr<const AbstractNode> &root)
{
  this->root_node = root;
  this->nodecachemap.clear();
}

void Tree::setDocumentPath(const std::string& path){
  this->document_path = path;
}

const std::string Tree::getDocumentPath() const
{
  return this->document_path;
}
