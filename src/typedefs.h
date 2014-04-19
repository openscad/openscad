#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class Expression> ExpressionSP;
class Assignment : public std::pair<std::string, ExpressionSP >
{
public:
    Assignment(std::string name) : std::pair<std::string, ExpressionSP>(name, ExpressionSP()) {}
    Assignment(std::string name, boost::shared_ptr<class Expression> expr) : std::pair<std::string, ExpressionSP>(name, expr) {}
};

typedef std::vector<Assignment> AssignmentList;
typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;
#endif
