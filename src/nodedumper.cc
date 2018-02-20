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
		this->cache.insert_start(node);
		
		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		dump << node << " {\n";
		this->currindent++;
		
		if (this->idprefix) dump << "n" << node.index() << ":";

	} else if (state.isPostfix()) {

		this->currindent--;
		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		dump << "};";

		// insert end index
		this->cache.insert_end(node);
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
		this->cache.set_root_stream(this->dump);
		std::stringstream &dump = *this->dump;

		// FIXME mostly duplicated from Response NodeDumper::visit(State &state, const AbstractNode &node)
		// not sure I can call directly or if that falsely increments the AbstractNode idx 
		if (node.modinst->isBackground()) dump << "%";
		if (node.modinst->isHighlight()) dump << "#";

		// insert start index
		this->cache.insert_start(node);

		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		dump << "root() {\n";
		this->currindent++;

		if (this->idprefix) dump << "n" << node.index() << ":";

	} else if (state.isPostfix()) {
		std::stringstream &dump = *this->dump;

		this->currindent--;
		for(int i = 0; i < this->currindent; ++i) {
			dump << this->indent;
		}
		dump << "};";

		// insert end index
		this->cache.insert_end(node);
	}

	return Response::ContinueTraversal;
}
