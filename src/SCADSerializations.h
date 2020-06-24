#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include "pcache.h"
#include "polyset.h"
BOOST_CLASS_EXPORT(PolySet);
#include "Polygon2d.h"
BOOST_CLASS_EXPORT(Polygon2d);
#include "Geometry.h"
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Geometry);

BOOST_SERIALIZATION_SPLIT_FREE(PolySet);
BOOST_SERIALIZATION_SPLIT_FREE(Polygon2d);
BOOST_SERIALIZATION_SPLIT_FREE(Geometry);

namespace boost{
namespace serialization{
#ifdef ENABLE_HIREDIS

template <class Archive>
void serialize(Archive &ar, PCache::CGAL_cache_entry &ce, const unsigned int ){
    ar & ce.N;
    ar & ce.msg;
}

template <class Archive>
void serialize(Archive &ar, PCache::Geom_cache_entry &ce, const unsigned int){
    ar & ce.geom;
    ar & ce.msg;
}

#endif // ENABLE_HIREDIS
template <class Archive>
void serialize(Archive &ar, Vector3d &v, const unsigned int){
    ar & v(0);
    ar & v(1);
    ar & v(2);
}

template <class Archive>
void serialize(Archive &ar, Vector2d &v, const unsigned int){
    ar & v(0);
    ar & v(1);
}

template<class Archive>
void serialize(Archive &ar, Outline2d &o, const unsigned int){
    ar & o.vertices;
    ar & o.positive;
}

template<class Archive>
void save(Archive &ar, const Geometry& g, const unsigned int){
    unsigned int tmp = g.getConvexity();
    ar & tmp;
}
template<class Archive>
void load(Archive &ar,  Geometry& g, const unsigned int){
    unsigned int tmp ;
    tmp = g.getConvexity();
    ar & tmp;
}

template <class Archive>
void save(Archive &ar, const PolySet& ps, const unsigned int){
    ar & boost::serialization::base_object<Geometry> (ps);

    ar & ps.polygons;
    ar & ps.getPolygon();
    ar & ps.getDimension();
}
template<class Archive>
void load(Archive &ar, PolySet& ps, const unsigned int){
    ar & boost::serialization::base_object<Geometry> (ps);

    ar & ps.polygons;
    Polygon2d polygon;
    ar & polygon;
    ps.setPolygon(polygon);
    unsigned int dim;
    ar & dim;

    ps.setDim(dim);
    ps.setTrueDirty();
    ps.setUnknown();
}

template<class Archive>
void save(Archive &ar, const Polygon2d& p, const unsigned int){
    ar & boost::serialization::base_object<Geometry> (p);

    ar & p.outlines();
    ar & p.isSanitized();
}
template<class Archive>
void load(Archive &ar, Polygon2d& p,const unsigned int){
    ar & boost::serialization::base_object<Geometry> (p);

    Polygon2d::Outlines2d o;
    ar & o;
    p.setOutlines(o);
    bool sanitized;
    ar & sanitized;
    p.setSanitized(sanitized);
}

} // namespace serialization
} // namespace boost
