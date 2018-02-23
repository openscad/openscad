#pragma once

#include "nodecache.h"
#include <map>

/*!  
	For now, just an abstraction of the node tree which keeps a dump
	cache based on node indices around.

	Note that since node trees don't survive a recompilation, the tree cannot either.
 */
class Tree
{
public:
	Tree(const AbstractNode *root = nullptr) : root_node(root) {}
	~Tree();

	void setRoot(const AbstractNode *root);
	const AbstractNode *root() const { return this->root_node; }

	const std::string getString(const AbstractNode &node, const std::string &indent) const;
	const std::string getIdString(const AbstractNode &node) const;

private:
	const AbstractNode *root_node;
	// keep a separate nodecache per tuple of NodeDumper constructor parameters
	mutable std::map<std::tuple<std::string, bool, bool>, NodeCache>  nodecachemap;
};
