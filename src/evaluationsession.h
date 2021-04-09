#pragma once

#include <string>
#include <vector>

class Context;

class EvaluationSession
{
public:
	EvaluationSession(const std::string& documentRoot):
		document_root(documentRoot)
	{}
	
	void push_context(Context* context);
	void pop_context();
	
	const std::vector<Context*>& getStack() { return stack; }
	
	const std::string& documentRoot() const { return document_root; }

private:
	std::string document_root;
	std::vector<Context*> stack;
};
