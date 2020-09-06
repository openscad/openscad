#pragma once
#include <string>

#include "printutils.h"
#include "memory.h"
#include "cgalutils.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

#if BOOST_VERSION > 105800

class LCache
{
public:
  LCache();

  bool insert(const std::string &key, const std::string &serializedgeom);
  bool get(const std::string &key, std::string &serializedgeom);
  bool contains(const std::string &key);
  bool insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N);
  bool insertGeometry(const std::string &key, const shared_ptr<const Geometry> &geom);
  shared_ptr<const class CGAL_Nef_polyhedron> getCGAL(const std::string &key);
  shared_ptr<const class Geometry> getGeometry(const std::string &key);
  bool containsCGAL(const std::string &key);
  bool containsGeom(const std::string &key);
  std::string getHash(const std::string key);
  virtual ~LCache() {}

private:
  std::string path;
};
#else
class LCache
{
public:
  LCache() {}
};
#endif // if BOOST_VERSION > 105800
