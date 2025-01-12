#include "geometry/Geometry.h"
#include "utils/printutils.h"
#include <sstream>
#include <memory>
#include <boost/foreach.hpp>
#include <cstddef>
#include <string>
#include <utility>

GeometryList::GeometryList(Geometry::Geometries geometries) : children(std::move(geometries))
{
}

std::unique_ptr<Geometry> GeometryList::copy() const
{
  return std::make_unique<GeometryList>(*this);
}

size_t GeometryList::memsize() const
{
  size_t sum = 0;
  for (const auto& item : this->children) {
    sum += item.second->memsize();
  }
  return sum;
}

BoundingBox GeometryList::getBoundingBox() const
{
  BoundingBox bbox;
  for (const auto& item : this->children) {
    bbox.extend(item.second->getBoundingBox());
  }
  return bbox;
}

std::string GeometryList::dump() const
{
  std::stringstream out;
  for (const auto& item : this->children) {
    out << item.second->dump();
  }
  return out.str();
}

unsigned int GeometryList::getDimension() const
{
  unsigned int dim = 0;
  for (const auto& item : this->children) {
    if (!dim) dim = item.second->getDimension();
    else if (dim != item.second->getDimension()) {
      LOG(message_group::Warning, "Mixing 2D and 3D objects is not supported.");
      break;
    }
  }
  return dim;
}

bool GeometryList::isEmpty() const
{
  for (const auto& item : this->children) {
    if (!item.second->isEmpty()) return false;
  }
  return true;
}

void flatten(const GeometryList& geomlist, GeometryList::Geometries& childlist)
{
  for (const auto& item : geomlist.getChildren()) {
    if (const auto chlist = std::dynamic_pointer_cast<const GeometryList>(item.second)) {
      flatten(*chlist, childlist);
    } else {
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
