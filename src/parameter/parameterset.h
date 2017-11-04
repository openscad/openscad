#pragma once

#include <vector>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

class ParameterSet
{
public:
	static std::string parameterSetsKey;
	static std::string fileFormatVersionKey;
	static std::string fileFormatVersionValue;

protected:
	pt::ptree root;

public:
	ParameterSet() {}
	~ParameterSet() {}
	boost::optional<pt::ptree &> parameterSets();
	std::vector<std::string> getParameterNames();
	boost::optional<pt::ptree &> getParameterSet(const std::string &setName);
	void addParameterSet(const std::string setName, const pt::ptree &set);
	bool readParameterSet(const std::string &filename);
	void writeParameterSet(const std::string &filename);
	void applyParameterSet(class FileModule *fileModule, const std::string &setName);
};
