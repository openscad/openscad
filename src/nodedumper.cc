#include "nodedumper.h"
#include "state.h"
#include "module.h"
#include "ModuleInstantiation.h"

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
	return this->cache.contains(node);
}

/*!
	Indent or deindent. Must be called before we output any children.
*/
void NodeDumper::handleIndent(const State &state)
{
	if (state.isPrefix()) {
		this->currindent++;
	}
	else if (state.isPostfix()) {
		this->currindent--;
	}
}

/*!
	Dumps the block of children contained in this->visitedchildren,
	including braces and indentation.
	All children are assumed to be cached already.
 */
void NodeDumper::dumpChildBlock(const AbstractNode &node, std::stringstream &dump) const
{
	const auto &it = this->visitedchildren.find(node.index());

	if (it == this->visitedchildren.end() || it->second.empty()) {
		dump << ";";
	} else {
		dump << " {\n";
		dumpChildren(node, dump);
		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		dump << "}";
	}
}

void NodeDumper::dumpChildren(const AbstractNode &node, std::stringstream &dump) const
{
	const auto &it = this->visitedchildren.find(node.index());
	if (it == this->visitedchildren.end()) return;
	
	for (auto child : it->second) {
		assert(isCached(*child));
		const auto &str = this->cache[*child];
		if (!str.empty()) {
			if (child->modinst->isBackground()) dump << "%";
			if (child->modinst->isHighlight()) dump << "#";
			dump << str << "\n";
		}
	}
}

/*!
	Called for each node in the tree.
	Will abort traversal if we're cached
*/
Response NodeDumper::visit(State &state, const AbstractNode &node)
{
	if (isCached(node)) return Response::PruneTraversal;

	handleIndent(state);
	if (state.isPostfix()) {
		std::stringstream dump;
		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		if (this->idprefix) dump << "n" << node.index() << ":";
		dump << node;
		dumpChildBlock(node, dump);
		this->cache.insert(node, dump.str());
	}

	handleVisitedChildren(state, node);
	return Response::ContinueTraversal;
}

/*!
	Handle root nodes specially: Only list children
*/
Response NodeDumper::visit(State &state, const RootNode &node)
{
	if (isCached(node)) return Response::PruneTraversal;

	if (state.isPostfix()) {
		std::stringstream dump;
		dumpChildren(node, dump);
		this->cache.insert(node, dump.str());
	}

	handleVisitedChildren(state, node);
	return Response::ContinueTraversal;
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
