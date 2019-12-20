#pragma once

#include <string>
#include <vector>

#include "AST.h"
#include "memory.h"
#include "annotation.h"

class Assignment :  public ASTNode
{
public:
	Assignment(std::string name, const Location &loc)
				: ASTNode(loc), name(name) { }
	Assignment(std::string name,
						 shared_ptr<class Expression> expr = shared_ptr<class Expression>(),
						 const Location &loc = Location::NONE)
		: ASTNode(loc), name(name), expr(expr) { }
	// map an Assignment to a new name, for resolving arguments
	Assignment(std::string name, const Assignment& from)
				: ASTNode(from.loc), name(name), expr(from.expr) { }
	
	void print(std::ostream &stream, const std::string &indent) const override;

	virtual void addAnnotations(AnnotationList *annotations);
	virtual bool hasAnnotations() const;
	virtual const Annotation *annotation(const std::string &name) const;

	// FIXME: Make protected
	std::string name;
	shared_ptr<class Expression> expr;
protected:
	AnnotationMap annotations;
};
       
       
typedef std::vector<Assignment> AssignmentList;

// Type for "resolvedArguments".  Includes default argument expressions for those not provided in call.
//   size is at least that of arguments in the function/module definition
//   bool indicates context:
//     true  - use default value, evaluated from definition context.
//     false - use provided value, evaluated from context of call-site.
typedef std::vector<std::pair<bool, Assignment> > AssignmentMap;
