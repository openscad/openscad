#pragma once

#include <string>
#include <vector>

enum class inbuilt_keyword {
	CUBE,
	SPHERE,
	CYLINDER,
	POLYHEDRON,
	SQUARE,
	CIRCLE,
	POLYGON
};

class Keyword 
{
public:
	Keyword(inbuilt_keyword keyword);
	~Keyword();

	std::string word;
	std::vector<std::string> calltip;

private:
	const std::string getWord(inbuilt_keyword keyword);
	const std::vector<std::string> getCalltip(inbuilt_keyword keyword);
};
