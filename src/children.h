#pragma once

#include "context.h"
#include "localscope.h"

class AbstractNode;

class Children
{
public:
	Children(const LocalScope* children_scope, const std::shared_ptr<Context>& context);
	
	std::vector<AbstractNode*> instantiate();
	AbstractNode* instantiate(AbstractNode* target);
	
	bool empty() const { return !children_scope->hasChildren(); }

private:
	const LocalScope* children_scope;
	std::shared_ptr<Context> context;
};
