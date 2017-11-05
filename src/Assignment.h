#pragma once

#include <string>
#include <vector>
#include <utility>

#include "value.h"
#include "AST.h"
#include "memory.h"
#include "annotation.h"

class Assignment : public ASTNode
{
public:
	Assignment(std::string name, const Location &loc)
		: ASTNode(loc), name(name) { }
	Assignment(std::string name,
						 shared_ptr<class Expression> expr = shared_ptr<class Expression>(),
						 const Location &loc = Location::NONE)
		: ASTNode(loc), name(name), expr(expr) { }
	std::string name;
	shared_ptr<class Expression> expr;

	virtual void addAnnotations(AnnotationList *annotations);
	virtual bool hasAnnotations() const;
	virtual const Annotation *annotation(const std::string &name) const;

protected:
	AnnotationMap annotations;
};


typedef std::vector<Assignment> AssignmentList;
typedef std::unordered_map<std::string, const Expression *> AssignmentMap;
