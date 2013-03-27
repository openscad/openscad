#include "Polygon2d.h"
#include <boost/foreach.hpp>

size_t Polygon2d::memsize() const
{
	size_t mem = 0;
	BOOST_FOREACH(const Outline2d &o, this->outlines) {
		mem += o.size() * sizeof(Vector2d) + sizeof(Outline2d);
	}
	mem += sizeof(Polygon2d);
	return mem;
}

BoundingBox Polygon2d::getBoundingBox() const
{
	BoundingBox bbox;
	BOOST_FOREACH(const Outline2d &o, this->outlines) {
		BOOST_FOREACH(const Vector2d &v, o) {
			bbox.extend(Vector3d(v[0], v[1], 0));
		}
	}
	return bbox;
}

std::string Polygon2d::dump() const
{
	std::stringstream out;
	out << "Polygon2d:";
//FIXME: Implement
/*
	  << "\n convexity:" << this->convexity
	  << "\n num polygons: " << polygons.size()
	  << "\n num borders: " << borders.size()
	  << "\n polygons data:";
	for (size_t i = 0; i < polygons.size(); i++) {
		out << "\n  polygon begin:";
		const Polygon *poly = &polygons[i];
		for (size_t j = 0; j < poly->size(); j++) {
			Vector3d v = poly->at(j);
			out << "\n   vertex:" << v.transpose();
		}
	}
	out << "\n borders data:";
	for (size_t i = 0; i < borders.size(); i++) {
		out << "\n  border polygon begin:";
		const Polygon *poly = &borders[i];
		for (size_t j = 0; j < poly->size(); j++) {
			Vector3d v = poly->at(j);
			out << "\n   vertex:" << v.transpose();
		}
	}
	out << "\nPolySet end";
*/
	return out.str();
}

// PolySet *Polygon2d::tessellate()
// {
// 	return NULL;
// }
