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

struct TraverseNode
{
	Traverser *traverser;
	const State &state;
	TraverseNode(Traverser *traverser, const State &state) : 
		traverser(traverser), state(state) {}
	void operator()(const AbstractNode *node) { traverser->traverse(*node, state); }
};

void Traverser::traverse(const AbstractNode &node, const State &state)
{
	// FIXME: Handle abort
	
	State newstate = state;
	newstate.setNumChildren(node.getChildren().size());
	
	if (traversaltype == PREFIX || traversaltype == PRE_AND_POSTFIX) {
		newstate.setPrefix(true);
		newstate.setParent(state.parent());
		node.accept(newstate, this->visitor);
	}
	
	newstate.setParent(&node);
	std::for_each(node.getChildren().begin(), node.getChildren().end(), TraverseNode(this, newstate));
	
	if (traversaltype == POSTFIX || traversaltype == PRE_AND_POSTFIX) {
		newstate.setParent(state.parent());
		newstate.setPrefix(false);
		newstate.setPostfix(true);
		node.accept(newstate, this->visitor);
	}
}
