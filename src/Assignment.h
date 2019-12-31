#pragma once

#include <string>
#include <vector>

#include "AST.h"
#include "memory.h"
#include "annotation.h"

class Assignment :  public ASTNode
{
public:
	Assignment(std::string name, const Location &loc, bool isOverride = false)
		: Assignment(name, {}, loc, isOverride) { }
	Assignment(std::string name,
						 shared_ptr<class Expression> expr = shared_ptr<class Expression>(),
						 const Location &loc = Location::NONE,
						 bool isOverride = false)
		: ASTNode(loc), name(name), expr(expr), isOverride(isOverride) { }
	
	void print(std::ostream &stream, const std::string &indent) const override;

	virtual void addAnnotations(AnnotationList *annotations);
	virtual bool hasAnnotations() const;
	virtual const Annotation *annotation(const std::string &name) const;

	// FIXME: Make protected
	std::string name;
	shared_ptr<class Expression> expr;
	bool isOverride; // True for assignments overridden on the cmd-line
	bool isDisabled{false}; // True for assignments that was disabled, typically after being
	                 // used to replace a previous assignment to the same variable
protected:
	AnnotationMap annotations;
};
       
       
typedef std::vector<shared_ptr<Assignment>> AssignmentList;
typedef std::unordered_map<std::string, const Expression*> AssignmentMap;
