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
	
	void print(std::ostream &stream, const std::string &indent) const override;
	const std::string& getName() const { return name; };
	const shared_ptr<Expression>& getExpr() const { return expr; };
	// setExpr used by customizer parameterobject etc.
	void setExpr(shared_ptr<Expression> e) { expr = std::move(e); };

	virtual void addAnnotations(AnnotationList *annotations);
	virtual bool hasAnnotations() const;
	virtual const Annotation *annotation(const std::string &name) const;

protected:
	const std::string name;
	shared_ptr<class Expression> expr;
	AnnotationMap annotations;
};

template<class... Args> shared_ptr<Assignment> assignment(Args... args) {
	return make_shared<Assignment>(args...);
}
       
typedef std::vector<shared_ptr<Assignment>> AssignmentList;
typedef std::unordered_map<std::string, const Expression*> AssignmentMap;
