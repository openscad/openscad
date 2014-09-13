#pragma once

#include <string>
#include <vector>
#include <utility>
#include <boost/shared_ptr.hpp>

class Assignment : public std::pair<std::string, boost::shared_ptr<class Expression> >
{
public:
    Assignment(std::string name) { first = name; second = boost::shared_ptr<class Expression>(); }
    Assignment(std::string name, boost::shared_ptr<class Expression> expr) { first = name; second = expr; }
};

typedef std::vector<Assignment> AssignmentList;
typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;
