#pragma once

#include "SourceFile.h"
#include "parametervirtualwidget.h"
#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, class ParameterObject *> entry_map_t;

class ParameterExtractor
{
public:
	ParameterExtractor();
	virtual ~ParameterExtractor();
	void setParameters(const SourceFile* file,entry_map_t& entries,std::vector<std::string>&,bool&);
	void applyParameters(SourceFile *file,entry_map_t&);
};
