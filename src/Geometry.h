#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <stddef.h>
#include <string>
#include "linalg.h"

class Geometry
{
public:
	virtual size_t memsize() const = 0;
	virtual BoundingBox getBoundingBox() const = 0;
	virtual std::string dump() const = 0;
};

#endif
