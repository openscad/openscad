#include "traverser.h"
#include "visitor.h"
#include "node.h"
#include "state.h"
#include <algorithm>

void Traverser::execute() 
{
	State state(NULL);
	traverse(this->root, state);
}

Response Traverser::traverse(const AbstractNode &node, const State &state)
{
	State newstate = state;
	newstate.setNumChildren(node.getChildren().size());
	
	Response response = ContinueTraversal;
	if (traversaltype == PREFIX || traversaltype == PRE_AND_POSTFIX) {
		newstate.setPrefix(true);
		newstate.setParent(state.parent());
		response = node.accept(newstate, this->visitor);
	}

	// Pruned traversals mean don't traverse children
	if (response == ContinueTraversal) {
		newstate.setParent(&node);
		for(const auto &chnode : node.getChildren()) {
			response = this->traverse(*chnode, newstate);
			if (response == AbortTraversal) return response; // Abort immediately
		}
	}

	// Postfix is executed for all non-aborted traversals
	if (response != AbortTraversal) {
		if (traversaltype == POSTFIX || traversaltype == PRE_AND_POSTFIX) {
			newstate.setParent(state.parent());
			newstate.setPrefix(false);
			newstate.setPostfix(true);
			response = node.accept(newstate, this->visitor);
		}
	}

	if (response != AbortTraversal) response = ContinueTraversal;
	return response;
}
