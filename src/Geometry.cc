#include "Geometry.h"
#include "printutils.h"
#include <boost/foreach.hpp>

GeometryList::GeometryList()
{
}

GeometryList::GeometryList(const Geometry::Geometries &geometries) : children(geometries)
{
}

GeometryList::~GeometryList()
{
}

size_t GeometryList::memsize() const
{
	size_t sum = 0;
	for (const auto &item : this->children) {
		sum += item.second->memsize();
	}
	return sum;
}

BoundingBox GeometryList::getBoundingBox() const
{
	BoundingBox bbox;
	for (const auto &item : this->children) {
		bbox.extend(item.second->getBoundingBox());
	}
	return bbox;
}

std::string GeometryList::dump() const
{
	std::stringstream out;
	for (const auto &item : this->children) {
		out << item.second->dump();
	}
	return out.str();
}

unsigned int GeometryList::getDimension() const
{
	unsigned int dim = 0;
	for (const auto &item : this->children) {
		if (!dim) dim = item.second->getDimension();
		else if (dim != item.second->getDimension()) {
			LOG(message_group::Warning,Location::NONE,"","Mixing 2D and 3D objects is not supported.");
			break;
		}
	}
	return dim;
}

bool GeometryList::isEmpty() const
{
	for (const auto &item : this->children) {
		if (!item.second->isEmpty()) return false;
	}
	return true;
}

void flatten(const GeometryList &geomlist, GeometryList::Geometries &childlist)
{
	for (const auto &item : geomlist.getChildren()) {
		if (const auto chlist = dynamic_pointer_cast<const GeometryList>(item.second)) {
			flatten(*chlist, childlist);
		}
		else {
			childlist.push_back(item);
		}
	}
}

/*!
	Creates a new GeometryList which has a flat hierarchy (all
	children directly reachable GeometryLists are collected in a flat
	list)
*/
Geometry::Geometries GeometryList::flatten() const
{
	Geometries newchildren;
	::flatten(*this, newchildren);
	return newchildren;
}
