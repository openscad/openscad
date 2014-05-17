#include "Geometry.h"
#include "printutils.h"
#include <boost/foreach.hpp>

GeometryList::GeometryList()
{
}

GeometryList::GeometryList(const Geometry::ChildList &chlist)
{
	BOOST_FOREACH(const Geometry::ChildItem &item, chlist) {
		this->children.push_back(item.second);
	}
}
	
GeometryList::GeometryList(const GeometryList::Geometries &chlist) : children(chlist)
{
}

GeometryList::~GeometryList()
{
}

size_t GeometryList::memsize() const
{
	size_t sum = 0;
	BOOST_FOREACH(const boost::shared_ptr<const Geometry> &geom, this->children) {
		sum += geom->memsize();
	}
	return sum;
}

BoundingBox GeometryList::getBoundingBox() const
{
	BoundingBox bbox;
	BOOST_FOREACH(const boost::shared_ptr<const Geometry> &geom, this->children) {
		bbox.extend(geom->getBoundingBox());
	}
	return bbox;
}

std::string GeometryList::dump() const
{
	std::stringstream out;
	BOOST_FOREACH(const boost::shared_ptr<const Geometry> &geom, this->children) {
		out << geom->dump();
	}
	return out.str();
}

unsigned int GeometryList::getDimension() const
{
	unsigned int dim = 0;
	BOOST_FOREACH(const boost::shared_ptr<const Geometry> &geom, this->children) {
		if (!dim) dim = geom->getDimension();
		else if (dim != geom->getDimension()) {
			PRINT("WARNING: Mixing 2D and 3D objects is not supported.");
			break;
		}
	}
    return dim;
}

bool GeometryList::isEmpty() const
{
	BOOST_FOREACH(const boost::shared_ptr<const Geometry> &geom, this->children) {
		if (!geom->isEmpty()) return false;
	}
	return true;
}

void flatten(const GeometryList *geomlist, GeometryList::Geometries &childlist)
{
	BOOST_FOREACH(const boost::shared_ptr<const Geometry> &geom, geomlist->getChildren()) {
		if (const GeometryList *chlist = dynamic_cast<const GeometryList *>(geom.get())) {
			flatten(chlist, childlist);
		}
		else {
			childlist.push_back(geom);
		}
	}
}

/*!
  Creates a new GeometryList which has a flat hierarchy (all
	children directly reachable GeometryLists are collected in a flat
	list)
*/
shared_ptr<GeometryList> GeometryList::flatten() const
{
	Geometries newchildren;
	::flatten(this, newchildren);
	return shared_ptr<GeometryList>(new GeometryList(newchildren));
}

