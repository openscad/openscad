#include "Tree.h"
#include "nodedumper.h"

#include <assert.h>
#include <algorithm>

/*!
	Returns the cached string representation of the subtree rooted by \a node.
	If node is not cached, the cache will be rebuilt.
*/
const std::string &Tree::getString(const AbstractNode &node) const
{
	assert(this->root_node);
	if (!this->nodecache.contains(node)) {
		NodeDumper dumper(this->nodecache, false);
		Traverser trav(dumper, *this->root_node, Traverser::PRE_AND_POSTFIX);
		trav.execute();
		assert(this->nodecache.contains(*this->root_node) &&
					 "NodeDumper failed to create a cache");
	}
	return this->nodecache[node];
}

/*!
	Sets a new root. Will clear the existing cache.
 */
void Tree::setRoot(const AbstractNode *root)
{
	this->root_node = root; 
	this->nodecache.clear();
}
