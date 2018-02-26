#include "nodedumper.h"
#include "state.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "memory.h"
#include <string>
#include <sstream>
#include <assert.h>
#include <boost/regex.hpp>

/*!
	\class NodeDumper

	A visitor responsible for creating a text dump of a node tree.  Also
	contains a cache for fast retrieval of the text representation of
	any node or subtree.
*/

void NodeDumper::initCache() 
{
	this->dumpstream.str("");
	this->dumpstream.clear();
	this->cache.clear();
}

void NodeDumper::finalizeCache()
{
	this->cache.setRootString(this->dumpstream.str());
}

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
	if (state.isPrefix()) {

		// For handling root modifier '!'
		// Check if we are processing the root of the current Tree and init cache
		if (this->root == &node) {
			this->initCache();
		}

		if (node.modinst->isBackground()) this->dumpstream << "%";
		if (node.modinst->isHighlight()) this->dumpstream << "#";

		// insert start index
		this->cache.insertStart(node.index(), this->dumpstream.tellp());
		
		if (this->idString) {
			
			const boost::regex re("[^\\s\\\"]+|\\\"(?:[^\\\"\\\\]|\\\\.)*\\\"");
			std::stringstream namestream;
			namestream << node;
			std::string name = namestream.str();
			boost::sregex_token_iterator it(name.begin(), name.end(), re, 0);
			std::copy(it, boost::sregex_token_iterator(), std::ostream_iterator<std::string>(this->dumpstream));
		
			if (node.getChildren().size() > 0) {
				this->dumpstream << "{";
			}

		} else {

			for(int i = 0; i < this->currindent; ++i) {
				this->dumpstream << this->indent;
			}
			this->dumpstream << node;
			if (node.getChildren().size() > 0) {
				this->dumpstream << " {\n";
			}
		}

		if (this->idprefix) this->dumpstream << "n" << node.index() << ":";

		this->currindent++;

	} else if (state.isPostfix()) {

		this->currindent--;
		
		if (this->idString) {
			if (node.getChildren().size() > 0) {
				this->dumpstream << "}";
			} else {
				this->dumpstream << ";";
			}
		} else {
			if (node.getChildren().size() > 0) {
				for(int i = 0; i < this->currindent; ++i) {
					this->dumpstream << this->indent;
				}
				this->dumpstream << "}\n";
			} else {
				this->dumpstream << ";\n";
			}
		}
	
		// insert end index
		this->cache.insertEnd(node.index(), this->dumpstream.tellp());

		// For handling root modifier '!'
		// Check if we are processing the root of the current Tree and finalize cache
		if (this->root == &node) {
			this->finalizeCache();
		}
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
		this->initCache();
		this->cache.insertStart(node.index(), this->dumpstream.tellp());
	} else if (state.isPostfix()) {
		this->cache.insertEnd(node.index(), this->dumpstream.tellp());
		this->finalizeCache();
	}

	return Response::ContinueTraversal;
}
