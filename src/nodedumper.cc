#include "nodedumper.h"
#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "state.h"
#include "nodecache.h"

#include <sstream>
#include <iostream>
#include <assert.h>

// For compatibility with old dump() output
#define NODEDUMPER_COMPAT_MODE
#ifdef NODEDUMPER_COMPAT_MODE
#include "dxflinextrudenode.h"
#include "dxfrotextrudenode.h"
#include "projectionnode.h"
#endif

NodeDumper *NodeDumper::global_dumper = NULL;

bool NodeDumper::isCached(const AbstractNode &node)
{
	return !this->cache[node].empty();
}

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

string NodeDumper::dumpChildren(const AbstractNode &node)
{
	std::stringstream dump;
	if (!this->visitedchildren[node.index()].empty()) {
		dump << " {\n";
		
		for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
				 iter != this->visitedchildren[node.index()].end();
				 iter++) {
// FIXME: assert that cache contains **iter
			dump << this->cache[**iter] << "\n";
		}
		
		dump << this->currindent << "}";
	}
	else {
#ifndef NODEDUMPER_COMPAT_MODE
		dump << ";";
#else
		if (dynamic_cast<const AbstractPolyNode*>(&node) &&
				!dynamic_cast<const ProjectionNode*>(&node) &&
				!dynamic_cast<const DxfRotateExtrudeNode*>(&node) &&
				!dynamic_cast<const DxfLinearExtrudeNode*>(&node)) dump << ";";
		else dump << " {\n" << this->currindent << "}";
#endif
	}
	return dump.str();
}


Response NodeDumper::visit(const State &state, const AbstractNode &node)
{
	if (isCached(node)) return PruneTraversal;
	else handleIndent(state);
	if (state.isPostfix()) {
		std::stringstream dump;
		dump << this->currindent << node;
		dump << dumpChildren(node);
		this->cache.insert(node, dump.str());
	}

	handleVisitedChildren(state, node);
	return ContinueTraversal;
}

const string &NodeDumper::getDump() const 
{ 
	assert(this->root); 
// FIXME: assert that cache contains root
	return this->cache[*this->root];
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
