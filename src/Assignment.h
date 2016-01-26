#pragma once

#include <string>
#include <vector>
#include <utility>

#include "AST.h"
#include "memory.h"

class Assignment : public ASTNode
{
public:
	Assignment(std::string name,
						 shared_ptr<class Expression> expr = shared_ptr<class Expression>())
		: name(name), expr(expr) {
	}
	
	std::string name;
	shared_ptr<class Expression> expr;
};

typedef std::vector<Assignment> AssignmentList;
