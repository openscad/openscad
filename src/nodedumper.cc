#include "nodedumper.h"
#include "state.h"
#include "module.h"

#include <string>
#include <sstream>
#include <assert.h>

/*!
	\class NodeDumper

	A visitor responsible for creating a text dump of a node tree.  Also
	contains a cache for fast retrieval of the text representation of
	any node or subtree.
*/

bool NodeDumper::isCached(const AbstractNode &node) const
{
	return !this->cache[node].empty();
}

/*!
	Indent or deindent. Must be called before we output any children.
*/
void NodeDumper::handleIndent(const State &state)
{
	if (state.isPrefix()) {
		this->currindent += "\t";
	}
	else if (state.isPostfix()) {
		this->currindent.erase((this->currindent.length() >= 1) ? 
													 this->currindent.length() - 1 : 0);
	}
}

/*!
	Dumps the block of children contained in this->visitedchildren,
	including braces and indentation.
	All children are assumed to be cached already.
 */
std::string NodeDumper::dumpChildren(const AbstractNode &node)
{
	std::stringstream dump;
	if (!this->visitedchildren[node.index()].empty()) {
		dump << " {\n";
		
		for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
				 iter != this->visitedchildren[node.index()].end();
				 iter++) {
			assert(isCached(**iter));
			if ((*iter)->modinst->isBackground()) dump << "%";
			if ((*iter)->modinst->isHighlight()) dump << "#";
			dump << this->cache[**iter] << "\n";
		}
		
		dump << this->currindent << "}";
	}
	else {
		dump << ";";
	}
	return dump.str();
}

/*!
	Called for each node in the tree.
	Will abort traversal if we're cached
*/
Response NodeDumper::visit(State &state, const AbstractNode &node)
{
	if (isCached(node)) return PruneTraversal;

	handleIndent(state);
	if (state.isPostfix()) {
		std::stringstream dump;
		dump << this->currindent;
		if (this->idprefix) dump << "n" << node.index() << ":";
		dump << node;
		dump << dumpChildren(node);
		this->cache.insert(node, dump.str());
	}

	handleVisitedChildren(state, node);
	return ContinueTraversal;
}

/*!
	Adds this given node to its parent's child list.
	Should be called for all nodes, including leaf nodes.
*/
void NodeDumper::handleVisitedChildren(const State &state, const AbstractNode &node)
{
	if (state.isPostfix()) {
		this->visitedchildren.erase(node.index());
		if (!state.parent()) {
			this->root = &node;
		}
		else {
			this->visitedchildren[state.parent()->index()].push_back(&node);
		}
	}
}
