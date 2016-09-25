#ifndef PARAMETERSET_H
#define PARAMETERSET_H

#include "expression.h"
#include "FileModule.h"
#include "modcontext.h"

#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class ParameterSet
{
protected:
	pt::ptree root;
	typedef std::map<std::string, pt::ptree::value_type> Parameterset;
	Parameterset parameterSet;

public:
	ParameterSet();
	~ParameterSet();
	void getParameterSet(const std::string &filename);
	void writeParameterSet(const std::string &filename);
	void applyParameterSet(FileModule *fileModule, const std::string &setName);

};

#endif // PARAMETERSET_H
