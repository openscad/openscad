#include "Tree.h"
#include "nodedumper.h"
#include "printutils.h"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <boost/regex.hpp>

Tree::~Tree()
{
	this->nodecache.clear();
	this->nodeidcache.clear();
}

/*!
	Returns the cached string representation of the subtree rooted by \a node.
	If node is not cached, the cache will be rebuilt.
*/
const std::string &Tree::getString(const AbstractNode &node) const
{
	assert(this->root_node);
	if (!this->nodecache.contains(node)) {
		this->nodecache.clear();
		this->nodeidcache.clear();
		NodeDumper dumper(this->nodecache, false);
		dumper.traverse(*this->root_node);
		assert(this->nodecache.contains(*this->root_node) &&
					 "NodeDumper failed to create a cache");
	}
	return this->nodecache[node];
}

/*!
	Returns the cached ID string representation of the subtree rooted by \a node.
	If node is not cached, the cache will be rebuilt.

	The difference between this method and getString() is that the ID string
	is stripped for whitespace. Especially indentation whitespace is important to
	strip to enable cache hits for equivalent nodes from different scopes.
*/
const std::string &Tree::getIdString(const AbstractNode &node) const
{
	assert(this->root_node);

	if (!this->nodeidcache.contains(node)) {
		const std::string &nodestr = getString(node);
		const boost::regex re("[^\\s\\\"]+|\\\"(?:[^\\\"\\\\]|\\\\.)*\\\"");
		std::stringstream sstream;
		boost::sregex_token_iterator i(nodestr.begin(), nodestr.end(), re, 0);
		std::copy(i, boost::sregex_token_iterator(), std::ostream_iterator<std::string>(sstream));

		const auto &result = this->nodeidcache.insert(node, sstream.str());
		PRINTDB("Id Cache MISS: %s", result);
		return result;
	} else {
		const auto & result = this->nodeidcache[node];
		PRINTDB("Id Cache HIT:  %s", result);
		return result;
	}
}

/*!
	Sets a new root. Will clear the existing cache.
 */
void Tree::setRoot(const AbstractNode *root)
{
	this->root_node = root; 
	this->nodecache.clear();
}
