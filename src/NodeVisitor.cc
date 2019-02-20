#include "NodeVisitor.h"
#include "state.h"

State NodeVisitor::nullstate(nullptr);

Response NodeVisitor::traverse(const AbstractNode &node, const State &state)
{
	State newstate = state;
	newstate.setNumChildren(node.getChildren().size());
	
	Response response = Response::ContinueTraversal;
	newstate.setPrefix(true);
	newstate.setParent(state.parent());
	response = node.accept(newstate, *this);

	// Pruned traversals mean don't traverse children
	if (response == Response::ContinueTraversal) {
		newstate.setParent(&node);
		for(const auto &chnode : node.getChildren()) {
			response = this->traverse(*chnode, newstate);
			if (response == Response::AbortTraversal) return response; // Abort immediately
		}
	}

	// Postfix is executed for all non-aborted traversals
	if (response != Response::AbortTraversal) {
		newstate.setParent(state.parent());
		newstate.setPrefix(false);
		newstate.setPostfix(true);
		response = node.accept(newstate, *this);
	}

	if (response != Response::AbortTraversal) response = Response::ContinueTraversal;
	return response;
}
