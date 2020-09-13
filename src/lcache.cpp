#include <sstream>
#include <fstream>
#include <functional>
#include <streambuf>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem.hpp>

#include "PlatformUtils.h"
#include "printutils.h"
#include "lcache.h"
#include "pcache.h"
#include "SCADSerializations.h"


namespace fs = boost::filesystem;

#if BOOST_VERSION > 105800

namespace {

std::string getHash(const std::string& key) {
  const std::hash<std::string> string_hash;
  const auto i = string_hash(key);
  return STR(std::uppercase << std::setfill('0') << std::setw(16) << std::hex << i);
}

std::pair<fs::path, fs::path> getPath(const std::string& path, const std::string &prefix, const std::string& key) {
  const auto hash = getHash(key);
  return std::make_pair(fs::path(path) / fs::path(prefix + hash.substr(0, 2)), fs::path(hash.substr(2)));
}

bool exists(const std::pair<fs::path, fs::path>& filePath, bool create = false) {
  if (!fs::exists(std::get<0>(filePath))) {
    fs::create_directories(std::get<0>(filePath));
    return false;
  }
  return fs::exists(std::get<0>(filePath) / std::get<1>(filePath));
}

std::string fullPath(const std::pair<fs::path, fs::path>& filePath) {
  auto fullPath = (std::get<0>(filePath) / std::get<1>(filePath)).string();
  return fullPath;
}

}

const std::string LCache::prefixCGAL = "c";
const std::string LCache::prefixGEOM = "g";

LCache::LCache() {
  path = PlatformUtils::localCachePath();
  if ((!fs::exists(path)) && (!PlatformUtils::createLocalCachePath())) {
    PRINTB("Error: Cannot create cache path: %s", path);
  }
}

bool LCache::insert(const std::string &prefix, const std::string &key, const std::string &serializedgeom) {
  const std::pair<fs::path, fs::path> filePath = getPath(path, prefix, key);
  if (!exists(filePath, true)) {
    std::ofstream fstream(fullPath(filePath));
    fstream << serializedgeom;
    fstream.close();
  }
  else {
    PRINTDB("Cache file already exists %s", key);
  }
  return true;
}

bool LCache::get(const std::string &prefix, const std::string &key, std::string &serializedgeom) {
  const std::pair<fs::path, fs::path> filePath = getPath(path, prefix, key);
  std::ifstream fstream(fullPath(filePath));
  if (!fstream.is_open()) {
    PRINTDB("Cannot open Cache file: %s", key);
    fstream.close();
    return false;
  }
  std::string data((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
  serializedgeom = data;
  fstream.close();
  return true;
}

bool LCache::contains(const std::string &prefix, const std::string &key) {
  const std::pair<fs::path, fs::path> filePath = getPath(path, prefix, key);
  return exists(filePath);
}

bool LCache::insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N){
  PRINTDB("add CGAL: %s", key.c_str());
  std::stringstream ss1;
  if (N->p3 != nullptr) {
    std::stringstream ss;
    ss << *N->p3;
    std::string data = ss.str();
    CGAL_cache_entry ce(data);
    ss.clear();
    boost::archive::text_oarchive oa(ss1);
    oa << ce;
  }
  return insert(prefixCGAL, key, ss1.str());
}

bool LCache::insertGeometry(const std::string &key, const shared_ptr<const Geometry> &geom){
  PRINTDB("add GEOM: %s", key.c_str());
  std::stringstream ss;
  if (geom != nullptr) {
    if (!geom->serializable()) {
      return false;
    }
    Geom_cache_entry ce(geom);
    boost::archive::text_oarchive oa(ss);
    oa << ce;
  }
  return insert(prefixGEOM, key, ss.str());
}

shared_ptr<const CGAL_Nef_polyhedron> LCache::getCGAL(const std::string &key){
  PRINTDB("get CGAL: %s", key.c_str());
  std::string data;
  if (get(prefixCGAL, key, data) && !data.empty()) {
    shared_ptr<CGAL_Nef_polyhedron3> p3(new CGAL_Nef_polyhedron3());
    CGAL_cache_entry ce;
    std::stringstream ss(data);
    boost::archive::text_iarchive io(ss);
    io >> ce;
    std::string n = ce.N;
    std::stringstream ss1(n);
    ss1 >> *p3;
    shared_ptr<const CGAL_Nef_polyhedron> N(new CGAL_Nef_polyhedron(p3));
    return N;
  }
  shared_ptr<const CGAL_Nef_polyhedron> N(new CGAL_Nef_polyhedron());
  return N;
}

shared_ptr<const Geometry> LCache::getGeometry(const std::string &key){
  PRINTDB("get GEOM: %s", key.c_str());
  std::string data;
  shared_ptr<const Geometry> geom;
  Geom_cache_entry ce;
  if (get(prefixGEOM, key, data) && !data.empty()) {
    std::stringstream ss(data);
    boost::archive::text_iarchive io(ss);
    io >> ce;
    geom = ce.geom;
  }
  return geom;
}

bool LCache::containsCGAL(const std::string &key){
  const auto result = contains(prefixCGAL, key);
  PRINTDB("[%s] CGAL: %s", result % key.c_str());
  return result;
}

bool LCache::containsGeom(const std::string &key){
  const auto result = contains(prefixGEOM, key);
  PRINTDB("[%s] GEOM: %s", result % key.c_str());
  return result;
}

#endif // if BOOST_VERSION > 105800
