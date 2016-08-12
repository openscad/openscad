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
    void updateLoc(const Location & loc);
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
protected:
    Annotation(const std::string name, const AssignmentList assignments, const AssignmentList args);
    
public:
    virtual ~Annotation();

    std::string dump() const;
    const std::string & get_name();
    virtual ValuePtr evaluate(class Context *ctx, const std::string& var) const;
    
    Annotation& operator=(const Annotation&);

    static const Annotation * create(const std::string name, const AssignmentList assignments);

//private:
    std::string name;
    /** Defines the names and values parsed from the script */
    AssignmentList assignments;
private:
    /** Defines the names of the positional parameters */
    AssignmentList args;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     	
