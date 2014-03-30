#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

class Assignment : public std::pair<std::string, boost::shared_ptr<class Expression> >
{
public:
    Assignment(std::string name) : pair(name, boost::shared_ptr<class Expression>()) {}
    Assignment(std::string name, boost::shared_ptr<class Expression> expr) : pair(name, expr) {}
};

typedef std::vector<Assignment> AssignmentList;
typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;

#endif
