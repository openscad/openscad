#include "nodedumper.h"
#include "state.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "memory.h"
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
	Called for each node in the tree.
	Will abort traversal if we're cached
*/
Response NodeDumper::visit(State &state, const AbstractNode &node)
{
	std::stringstream& dump = *this->dump;
	if (state.isPrefix()) {
		if (node.modinst->isBackground()) dump << "%";
		if (node.modinst->isHighlight()) dump << "#";

		// insert start index
		this->cache.insert_start(node, dump.tellp());
		
		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		
		dump << node;
		if (node.getChildren().size() > 0) 
			dump << " {\n";
		this->currindent++;
		
		if (this->idprefix) dump << "n" << node.index() << ":";

	} else if (state.isPostfix()) {

		this->currindent--;
		
		if (node.getChildren().size() > 0) {
			for(int i = 0; i < this->currindent; ++i) {
				dump << this->indent;
			}
			dump << "}\n";
		} else {
			dump << ";\n";
		}
		
		// insert end index
		this->cache.insert_end(node, dump.tellp());
	}

	return Response::ContinueTraversal;
}

/*!
	Handle root nodes specially: Only list children
*/
Response NodeDumper::visit(State &state, const RootNode &node)
{
	if (isCached(node)) return Response::PruneTraversal;

	if (state.isPrefix()) {
		this->cache.clear();
		// make a fresh stringstream
		this->dump = std::make_shared<std::stringstream>();
		std::stringstream &dump = *this->dump;

		// insert start index
		this->cache.insert_start(node, dump.tellp());

	} else if (state.isPostfix()) {
		std::stringstream &dump = *this->dump;
		// insert end index
		this->cache.insert_end(node, dump.tellp());
		// finalize cache
		this->cache.set_root_string(dump.str());
	}

	return Response::ContinueTraversal;
}
