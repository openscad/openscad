#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <stddef.h>
#include <string>
#include "linalg.h"

class Geometry
{
public:
	virtual ~Geometry() {}

	virtual size_t memsize() const = 0;
	virtual BoundingBox getBoundingBox() const = 0;
	virtual std::string dump() const = 0;
	virtual unsigned int getDimension() const = 0;
	virtual unsigned int getConvexity() const { return 1; };
};

#endif
