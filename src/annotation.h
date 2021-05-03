#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "memory.h"

class Annotation
{
public:
	Annotation(const std::string &name, shared_ptr<class Expression> expr);
	
	void print(std::ostream &stream, const std::string &indent) const;
	const std::string &getName() const { return name; }
	const shared_ptr<Expression>& getExpr() const { return expr; }
	
private:
	std::string name;
	shared_ptr<Expression> expr;
};

typedef std::vector<Annotation> AnnotationList;
typedef std::unordered_map<std::string, Annotation *> AnnotationMap;
