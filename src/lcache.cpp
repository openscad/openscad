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

LCache::LCache() {
    path = PlatformUtils::localCachePath();
}

bool LCache::insert(const std::string& key, const std::string& serializedgeom) {
    if ((!fs::exists(path)) && (!PlatformUtils::createLocalCachePath())) {
        PRINTB("Error: Cannot create cache path: %s", path);
        return false;
    }
    std::string hased_file = getHash(key);
    fs::path new_path = fs::path(path) / fs::path(hased_file);

    if(!fs::exists(new_path)){
        std::ofstream fstream(new_path.string());
        fstream << serializedgeom;
        fstream.close();
    } else {
        PRINTDB("Cache file already exists %s", key);
    }
    return true;
}

bool LCache::get(const std::string &key, std::string &serializedgeom) {
    std::string hased_file = getHash(key);
    fs::path new_path = fs::path(path) / fs::path(hased_file);

    std::ifstream fstream(new_path.string());
    if(!fstream.is_open()) {
        PRINTDB("Cannot open Cache file: %s", key);
        fstream.close();
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
    serializedgeom = data;
    fstream.close();
    return true;
}

bool LCache::contains(const std::string &key) {
    std::string hased_file = getHash(key);
    fs::path new_path = fs::path(path) / fs::path(hased_file);
    return fs::exists(new_path);
}

std::string LCache::getHash(const std::string key) {
        std::hash<std::string> string_hash;
        size_t i = string_hash(key);
        std::stringstream ss;
        ss << i;
        return ss.str();
}

bool LCache::insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N){
    PRINTDB("Insert: %s", key.c_str());
    std::stringstream ss;
    if(N->p3 == nullptr){
        return insert("CGAL-"+key, "");
    }
    ss << *N->p3;
    std::string data = ss.str();
    CGAL_cache_entry ce(data);
    ss.clear();
    std::stringstream ss1;
    boost::archive::text_oarchive oa(ss1);
    oa << ce;
    return insert("CGAL-"+key, ss1.str());
}

bool LCache::insertGeometry(const std::string &key, const shared_ptr<const Geometry> &geom){
    if(geom==nullptr){
        return insert("GEOM-"+key, "");
    }
    if(geom->serializable()){
        PRINTDB("Insert: %s", key.c_str());
        std::stringstream ss;
        Geom_cache_entry ce(geom);
        boost::archive::text_oarchive oa(ss);
        oa << ce;
        std::string data = ss.str();
        return insert("GEOM-"+key, data);
    }else return true;
}

shared_ptr<const CGAL_Nef_polyhedron> LCache::getCGAL(const std::string &key){
    PRINTDB("Get: %s", key.c_str());
    std::string data;
    if(get("CGAL-"+key, data) && !data.empty()){
        shared_ptr<CGAL_Nef_polyhedron3> p3(new CGAL_Nef_polyhedron3());
        CGAL_cache_entry ce;
        std::stringstream ss(data);
        boost::archive::text_iarchive io(ss);
        io >> ce;
        std::string n =ce.N;
        std::stringstream ss1(n);
        ss1 >> *p3;
        shared_ptr<const CGAL_Nef_polyhedron> N(new CGAL_Nef_polyhedron(p3));
        return N;
    }
    shared_ptr<const CGAL_Nef_polyhedron> N(new CGAL_Nef_polyhedron());
    return N;
}

shared_ptr<const Geometry> LCache::getGeometry(const std::string &key){
    PRINTDB("Get: %s", key.c_str());
    std::string data;
    shared_ptr<const Geometry> geom;
    Geom_cache_entry ce;
    if(get("GEOM-"+key, data) && !data.empty()){
        std::stringstream ss(data);
        boost::archive::text_iarchive io(ss);
        io >> ce;
        geom = ce.geom;
    }
    return geom;
}

bool LCache::containsCGAL(const std::string &key){
    PRINTDB("Contains: %s", key.c_str());
    return contains("CGAL-"+key);
}

bool LCache::containsGeom(const std::string &key){
    PRINTDB("Contains: %s", key.c_str());
    return contains("GEOM-"+key);
}

#endif
