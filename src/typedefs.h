#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <string>
#include <vector>

typedef std::pair<std::string, class Expression*> Assignment;
typedef std::vector<Assignment> AssignmentList;
typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;

#endif
