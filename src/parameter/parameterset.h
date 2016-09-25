#pragma once

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

class ParameterSet
{
protected:
	pt::ptree root;

public:
	ParameterSet() {}
	~ParameterSet() {}
	void readParameterSet(const std::string &filename);
	void writeParameterSet(const std::string &filename) const;
	void applyParameterSet(class FileModule *fileModule, const std::string &setName);
};
