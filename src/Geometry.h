#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <stddef.h>
#include <string>
#include <list>

#include "linalg.h"
#include "memory.h"

class Geometry
{
public:
  typedef std::pair<const class AbstractNode *, shared_ptr<const Geometry> > ChildItem;
  typedef std::list<ChildItem> ChildList;

	Geometry() : convexity(1) {}
	virtual ~Geometry() {}

	virtual size_t memsize() const = 0;
	virtual BoundingBox getBoundingBox() const = 0;
	virtual std::string dump() const = 0;
	virtual unsigned int getDimension() const = 0;
	virtual bool isEmpty() const = 0;

	unsigned int getConvexity() const { return convexity; }
	void setConvexity(int c) { this->convexity = c; }

protected:
	int convexity;
};

class GeometryList : public Geometry
{
public:
  typedef std::list<shared_ptr<const Geometry> > Geometries;
	Geometries children;

	GeometryList();
	GeometryList(const Geometry::ChildList &chlist);
	GeometryList(const GeometryList::Geometries &chlist);
	virtual ~GeometryList();

	virtual size_t memsize() const;
	virtual BoundingBox getBoundingBox() const;
	virtual std::string dump() const;
	virtual unsigned int getDimension() const;
	virtual bool isEmpty() const;

	const Geometries &getChildren() const { 
		return this->children;
	}

	shared_ptr<GeometryList> flatten() const;

};

#endif
