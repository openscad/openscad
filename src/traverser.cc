#include "traverser.h"
#include "visitor.h"
#include "node.h"
#include "state.h"

void Traverser::execute() 
{
	State state(NULL);
	traverse(this->root, state);
}

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
	const std::list<AbstractNode*> &children = node.getChildren();
	for (std::list<AbstractNode*>::const_iterator iter = children.begin();
			 iter != children.end();
			 iter++) {
		
		traverse(**iter, newstate);
	}
	
	if (traversaltype == POSTFIX || traversaltype == PRE_AND_POSTFIX) {
		newstate.setParent(state.parent());
		newstate.setPrefix(false);
		newstate.setPostfix(true);
		node.accept(newstate, this->visitor);
	}
}
