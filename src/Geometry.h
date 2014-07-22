#pragma once

#include <stddef.h>
#include <string>
#include <list>

#include "linalg.h"
#include "memory.h"

class Geometry
{
public:
  typedef std::pair<const class AbstractNode *, shared_ptr<const Geometry> > GeometryItem;
  typedef std::list<GeometryItem> Geometries;

	Geometry() : convexity(1) {}
	virtual ~Geometry() {}

	virtual size_t memsize() const = 0;
	virtual BoundingBox getBoundingBox() const = 0;
	virtual std::string dump() const = 0;
	virtual unsigned int getDimension() const = 0;
	virtual bool isEmpty() const = 0;
	virtual Geometry *copy() const = 0;

	unsigned int getConvexity() const { return convexity; }
	void setConvexity(int c) { this->convexity = c; }

protected:
	int convexity;
};

class GeometryList : public Geometry
{
public:
	Geometries children;

	GeometryList();
	GeometryList(const Geometry::Geometries &geometries);
	virtual ~GeometryList();

	virtual size_t memsize() const;
	virtual BoundingBox getBoundingBox() const;
	virtual std::string dump() const;
	virtual unsigned int getDimension() const;
	virtual bool isEmpty() const;
	virtual Geometry *copy() const { return new GeometryList(*this); };

	const Geometries &getChildren() const { 
		return this->children;
	}

	Geometries flatten() const;

};
