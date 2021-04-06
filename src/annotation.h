#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "memory.h"

class Annotation
{
public:
	Annotation(const std::string &name, shared_ptr<class Expression> expr);
	virtual ~Annotation();
	
	virtual void print(std::ostream &stream, const std::string &indent) const;
	const std::string &getName() const;
	virtual class Value evaluate(std::shared_ptr<class Context> ctx) const;
	
private:
	std::string name;
	shared_ptr<Expression> expr;
};

typedef std::vector<Annotation> AnnotationList;
typedef std::unordered_map<std::string, Annotation *> AnnotationMap;
