#pragma once

#include "FileModule.h"
#include "parametervirtualwidget.h"
#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, class ParameterObject *> entry_map_t;
	typedef std::map<std::string,std::string > group_Condition;
class ParameterExtractor
{
public:
	ParameterExtractor();
	virtual ~ParameterExtractor();
	void setParameters(const FileModule* module,entry_map_t& entries,group_Condition& groupCondition,std::vector<std::string>&,bool&);
	void applyParameters(FileModule *fileModule,entry_map_t&);
};
