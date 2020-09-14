#pragma once
#include <string>

#include "printutils.h"
#include "memory.h"
#include "cgalutils.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

#if BOOST_VERSION > 105800

struct file_path_t {
	boost::filesystem::path _dir;
	std::string _file;

  const boost::filesystem::path& dir() const { return _dir; }
  const boost::filesystem::path path() const { return _dir / _file; }
  const boost::filesystem::path unique_path() const { return boost::filesystem::unique_path(_dir / (_file + "_%%%%-%%%%-%%%%-%%%%")); }
  const std::string& dirname() const { return _dir.filename().string(); }

  bool dir_exists() const { return fs::exists(_dir); }
  bool path_exists() const { return fs::exists(_dir / _file); }
};

struct file_entry_t {
	file_path_t path;
	std::time_t time;
	uintmax_t size;
};

class LCache
{
public:
  LCache(LCache const &) = delete;
  LCache &operator=(LCache const &) = delete;

  bool insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N);
  bool insertGeometry(const std::string &key, const shared_ptr<const Geometry> &geom);
  shared_ptr<const class CGAL_Nef_polyhedron> getCGAL(const std::string &key);
  shared_ptr<const class Geometry> getGeometry(const std::string &key);
  bool containsCGAL(const std::string &key) const;
  bool containsGeometry(const std::string &key) const;

  static LCache& instance() {
	static LCache cache;
	return cache;
  }

private:
  LCache();
  bool insert(const std::string &prefix, const std::string &key, const std::string &serializedgeom);
  bool get(const std::string &prefix, const std::string &key, std::string &serializedgeom);
  bool contains(const std::string &prefix, const std::string &key) const;
  void cleanup();
  void status() const;

  boost::uintmax_t size;
  std::string path;
  std::vector<file_entry_t> entries;

  static const std::string prefixCGAL;
  static const std::string prefixGEOM;
  static const boost::uintmax_t cacheSizeLimit;
  static const boost::uintmax_t cacheSizeClean;
};
#else
class LCache
{
public:
  LCache() {}
};
#endif // if BOOST_VERSION > 105800
