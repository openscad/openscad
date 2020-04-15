#pragma once

struct JumpIndicatorData
{
	JumpIndicatorData(int linenr, int colnr, int nrofchar, std::string path,std::string name) 
	: linenr(linenr), colnr(colnr), nrofchar(nrofchar), path(path),name(name)
	{
	}

	~JumpIndicatorData()
	{
	}

	int linenr;
	int colnr;
	int nrofchar;
	std::string path;
    std::string name;
};