#include "Tree.h"
#include "nodedumper.h"

#include <assert.h>

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
