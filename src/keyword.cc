#include <assert.h>
#include "keyword.h"

Keyword::Keyword(inbuilt_keyword keyword)
{
	this->word = this->getWord(keyword);
	this->calltip = this->getCalltip(keyword);
}

Keyword::~Keyword()
{
}

const std::string Keyword::getWord(inbuilt_keyword keyword)
{
	switch(keyword)
	{
		case inbuilt_keyword::CUBE:
			return "cube";
		case inbuilt_keyword::SPHERE:
			return "sphere";
		case inbuilt_keyword::CYLINDER:
			return "cylinder";
		case inbuilt_keyword::POLYHEDRON:
			return "polyhedron";
		case inbuilt_keyword::SQUARE:
			return "square";
		case inbuilt_keyword::CIRCLE:
			return "circle";
		case inbuilt_keyword::POLYGON:
			return "polygon";
		default:
			assert(false && "Unknown Builtin keyword");
			return "unknown";
	}
}

const std::vector<std::string> Keyword::getCalltip(inbuilt_keyword keyword)
{
	switch(keyword)
	{
		case inbuilt_keyword::CUBE:
			return {
					"cube(size)",
					"cube([width, depth, height])",
					"cube(size = [width, depth, height], center = true)",
					};

		case inbuilt_keyword::SPHERE:
			return {
					"sphere(radius)",
					"sphere(r = radius)",
					"sphere(d = diameter)",
					};

		case inbuilt_keyword::CYLINDER:
			return {
					"cylinder(h, r1, r2)",
					"cylinder(h = height, r = radius, center = true)",
					"cylinder(h = height, r1 = bottom, r2 = top, center = true)",
					"cylinder(h = height, d = diameter, center = true)",
					"cylinder(h = height, d1 = bottom, d2 = top, center = true)",
					};

		case inbuilt_keyword::POLYHEDRON:
			return {
					"polyhedron(points, triangles, convexity)",
					};

		case inbuilt_keyword::SQUARE:
			return {
					"square(size, center = true)",
					"square([width,height], center = true)",
					};

		case inbuilt_keyword::CIRCLE:
			return {
					"circle(radius)",
					"circle(r = radius)",
					"circle(d = diameter)",
					};

		case inbuilt_keyword::POLYGON:
			return {
					"polygon([points])",
					"polygon([points], [paths])",
					};

		default:
			assert(false && "Unknown Builtin keyword");
			return {"unknown"};
	}
}
