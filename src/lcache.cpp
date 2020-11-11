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
#include "ext/picosha2/picosha2.h"

namespace fs = boost::filesystem;

#if BOOST_VERSION > 105800

namespace {

std::string getHash(const std::string& key) {
  std::string hash = picosha2::hash256_hex_string(key);
  return hash;
}

file_path_t getPath(const std::string& path, const std::string &prefix, const std::string& key) {
  const auto hash = getHash(key);
  return file_path_t{ fs::path(path) / fs::path(prefix + hash.substr(0, 2)), hash.substr(2) };
}

}

const std::string LCache::prefixCGAL = "c";
const std::string LCache::prefixGEOM = "g";
const boost::uintmax_t LCache::cacheSizeLimit = 10000000;
const boost::uintmax_t LCache::cacheSizeClean =  8000000;

LCache::LCache() {
  size = 0;
  path = PlatformUtils::localCachePath();
  if ((!fs::exists(path)) && (!PlatformUtils::createLocalCachePath())) {
	  LOG(message_group::Error, Location::NONE, "", "Cannot create cache path: %s", path);
  }

  cleanup();
}

bool LCache::insert(const std::string &prefix, const std::string &key, const std::string &serializedgeom) {
  const file_path_t filePath = getPath(path, prefix, key);
  if (!filePath.path_exists()) {
    fs::create_directory(filePath.dir());
    fs::path tempPath = filePath.unique_path();
    std::ofstream fstream(tempPath.string());
    fstream << serializedgeom;
    fstream.close();
    boost::system::error_code ec;
    fs::rename(tempPath, filePath.path(), ec);
    if (ec) {
      fs::remove(tempPath);
    }
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    entries.push_back(file_entry_t{filePath, now, serializedgeom.size()});
    size += serializedgeom.size();

    if (size > cacheSizeLimit) {
      cleanup();
    }
  }
  return true;
}

bool LCache::get(const std::string &prefix, const std::string &key, std::string &serializedgeom) {
  const file_path_t filePath = getPath(path, prefix, key);
  std::ifstream fstream(filePath.path().string());
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

bool LCache::contains(const std::string &prefix, const std::string &key) const {
  const file_path_t filePath = getPath(path, prefix, key);
  return filePath.path_exists();
}

void LCache::cleanup() {
  PRINTDB("del     : <%10d> start", size);
  for (const auto& entry : entries) {
    if (size < cacheSizeClean) {
      break;
    }
    boost::system::error_code ec;
    fs::remove(entry.path.path());
    fs::remove(entry.path.dir(), ec);
    size -= entry.size;
    const auto& path = entry.path;
    PRINTDB("del     : <%10d> %s/%s", size % path.dir().filename() % path.path().filename().string());
  }

  entries.clear();

  const size_t hashLen = 64;
  const size_t splitLen = 2;
  const size_t dirLen = prefixCGAL.size() + splitLen;
  const size_t fileLen = hashLen - splitLen;
  if (fs::is_directory(path)) {
    for (auto& dirEntry : boost::make_iterator_range(fs::directory_iterator(path), {})) {
      const auto dir = dirEntry.path().filename().string();
      if (fs::is_directory(dirEntry.path()) && dir.size() == dirLen) {
        for (auto& fileEntry : boost::make_iterator_range(fs::directory_iterator(dirEntry.path()), {})) {
          const auto file = fileEntry.path().filename().string();
          if (fs::is_regular(fileEntry.path()) && file.size() == fileLen) {
            const boost::uintmax_t fileSize = fs::file_size(fileEntry.path());
            entries.push_back(file_entry_t{file_path_t{dirEntry.path(), file}, fs::last_write_time(fileEntry.path()), fileSize});
            size += fileSize;
          }
        }
      }
    }
  }

  std::sort(entries.begin(), entries.end(), [](const file_entry_t& a, const file_entry_t& b) {
    return a.time < b.time;
  });
  PRINTDB("del     : <%10d> end", size);
}

void LCache::status() const {
  for (const auto e : entries) {
    std::cout << " " << e.path.path() << " (" << e.size << "/" << e.time << ")" << "\n";
  }
}

bool LCache::insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N){
  PRINTDB("add CGAL: <%10d> %s", size % key.c_str());
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
  PRINTDB("add GEOM: <%10d> %s", size % key.c_str());
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
  PRINTDB("get CGAL: <%10d> %s", size % key.c_str());
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
  PRINTDB("get GEOM: <%10d> %s", size % key.c_str());
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

bool LCache::containsCGAL(const std::string &key) const {
  const auto result = contains(prefixCGAL, key);
  PRINTDB("[%s] CGAL: <%10d> %s", result % size % key.c_str());
  return result;
}

bool LCache::containsGeometry(const std::string &key) const {
  const auto result = contains(prefixGEOM, key);
  PRINTDB("[%s] GEOM: <%10d> %s", result % size % key.c_str());
  return result;
}

#endif // if BOOST_VERSION > 105800
