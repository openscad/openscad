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
  virtual ~LCache() {}

  bool insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N);
  bool insertGeometry(const std::string &key, const shared_ptr<const Geometry> &geom);
  shared_ptr<const class CGAL_Nef_polyhedron> getCGAL(const std::string &key);
  shared_ptr<const class Geometry> getGeometry(const std::string &key);
  bool containsCGAL(const std::string &key);
  bool containsGeometry(const std::string &key);

private:
  bool insert(const std::string &prefix, const std::string &key, const std::string &serializedgeom);
  bool get(const std::string &prefix, const std::string &key, std::string &serializedgeom);
  bool contains(const std::string &prefix, const std::string &key);

  std::string path;

  static const std::string prefixCGAL;
  static const std::string prefixGEOM;
};
#else
class LCache
{
public:
  LCache() {}
};
#endif // if BOOST_VERSION > 105800
