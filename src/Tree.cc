#include "Tree.h"
#include "nodedumper.h"
#include "printutils.h"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <boost/regex.hpp>

Tree::~Tree()
{
	this->nodecachemap.clear();
}

/*!
	Returns the cached string representation of the subtree rooted by \a node.
	If node is not cached, the cache will be rebuilt.
*/
const std::string Tree::getString(const AbstractNode &node, const std::string &indent) const
{
	assert(this->root_node);
	NodeCache &nodecache = this->nodecachemap[indent];

	if (!nodecache.contains(node)) {
		NodeDumper dumper(nodecache, indent, false);
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
const std::string Tree::getIdString(const AbstractNode &node) const
{
	assert(this->root_node);
	std::string indent = "";
	NodeCache &nodecache = this->nodecachemap[indent];

	if (!nodecache.contains(node)) {
		nodecache.clear();
		NodeDumper dumper(nodecache, "", false);
		dumper.traverse(*this->root_node);
		assert(nodecache.contains(*this->root_node) &&
					 "NodeDumper failed to create id cache");
	}

	// FIXME: this is a HACK just to get things to compile and run
	// Should not run regex on every call
	// I think ideally we can have nodedumper take a parameter to generate pre-formatted idstring style
	const std::string &nodestr = nodecache[node];
	//                     [^\s\"]+|\"(?:[^\"\\]|\\.)*\"
	const boost::regex re("[^\\s\\\"]+|\\\"(?:[^\\\"\\\\]|\\\\.)*\\\"");
	std::stringstream sstream;
	boost::sregex_token_iterator i(nodestr.begin(), nodestr.end(), re, 0);
	std::copy(i, boost::sregex_token_iterator(), std::ostream_iterator<std::string>(sstream));

	return sstream.str();


}

/*!
	Sets a new root. Will clear the existing cache.
 */
void Tree::setRoot(const AbstractNode *root)
{
	this->root_node = root; 
	this->nodecachemap.clear();
}
