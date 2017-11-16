#pragma once

#include "FileModule.h"
#include "parametervirtualwidget.h"

class ParameterExtractor
{
protected:
	typedef std::map<std::string, class ParameterObject *> entry_map_t;
	entry_map_t entries;
	std::vector<std::string> ParameterPos;

public:
	ParameterExtractor();
	virtual ~ParameterExtractor();
	void setParameters(const FileModule* module,bool);
	void applyParameters(FileModule *fileModule);

protected:
	virtual void connectWidget() = 0;
	virtual void updateWidget() = 0;
	bool resetPara;
};
