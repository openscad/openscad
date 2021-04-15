#pragma once

#include "context.h"
#include "localscope.h"

class AbstractNode;
class ScopeContext;

class Children
{
public:
	Children(const LocalScope* children_scope, const std::shared_ptr<const Context>& context):
		children_scope(children_scope),
		context(context)
	{}
	
	Children(Children&& other) = default;
	Children& operator=(Children&& other) = default;
	
	AbstractNode* instantiate(AbstractNode* target) const;
	AbstractNode* instantiate(AbstractNode* target, const std::vector<size_t>& indices) const;
	AbstractNode* instantiate(size_t index) const;
	
	bool empty() const { return !children_scope->hasChildren(); }
	size_t size() const { return children_scope->moduleInstantiations.size(); }

private:
	const LocalScope* children_scope;
	std::shared_ptr<const Context> context;
	
	ContextHandle<ScopeContext> scopeContext() const;
};
