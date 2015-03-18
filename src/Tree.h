#pragma once

#include "nodecache.h"
#include "memory.h"

/*!  
	For now, just an abstraction of the node tree which keeps a dump
	cache based on node indices around.

	Note that since node trees don't survive a recompilation, the tree cannot either.
 */
class Tree
{
public:
	Tree() {}
	~Tree();

	void setRoot(const shared_ptr<AbstractNode> &root);
	const AbstractNode *root() const { return this->root_node.get(); }

	const std::string &getString(const AbstractNode &node) const;
	const std::string &getIdString(const AbstractNode &node) const;

private:
	shared_ptr<AbstractNode> root_node;
  mutable NodeCache nodecache;
  mutable NodeCache nodeidcache;
};
