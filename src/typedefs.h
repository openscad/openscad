#pragma once

#include <string>
#include <vector>
#include <utility>
#include "memory.h"

class Assignment : public std::pair<std::string, shared_ptr<class Expression>>
{
public:
    Assignment(std::string name,
							 shared_ptr<class Expression> expr = shared_ptr<class Expression>()) {
			first = name; second = expr;
		}
};

typedef std::vector<Assignment> AssignmentList;
typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;
