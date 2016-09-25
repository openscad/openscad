#pragma once

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "value.h"
#include "AST.h"
#include "memory.h"

class Annotation;

typedef std::map<std::string, Annotation *> AnnotationMap;

typedef std::vector<Annotation> AnnotationList;

class Assignment :  public ASTNode
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
    typedef std::vector<Assignment> AssignmentList;

    virtual void add_annotations(AnnotationList *annotations);
    virtual bool has_annotations() const;
    virtual const Annotation * annotation(const std::string &name) const;

protected:
    AnnotationMap annotations;
};
       
       
typedef std::vector<Assignment> AssignmentList;
       
class Annotation
{
public:
    Annotation(const std::string &name, shared_ptr<class Expression> expr);
    virtual ~Annotation();

    std::string dump() const;
    const std::string &getName() const;
    virtual ValuePtr evaluate(class Context *ctx) const;

private:
    std::string name;
    shared_ptr<Expression> expr;
};
