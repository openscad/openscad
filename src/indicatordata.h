#pragma once

struct IndicatorData
{
	IndicatorData(int linenr, int colnr, int nrofchar, std::string path) 
	: linenr(linenr), colnr(colnr), nrofchar(nrofchar), path(path)
	{
	}

	~IndicatorData()
	{
	}

	int linenr;
	int colnr;
	int nrofchar;
	std::string path;
};