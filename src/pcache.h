#pragma once
#include <string>

#include "printutils.h"
#include "memory.h"
#include "cgalutils.h"


#ifdef ENABLE_HIREDIS

#include <hiredis/hiredis.h>

class PCache
{
public:
    PCache();

    void init(const std::string _host, const unsigned int _port, const std::string _pass);
    bool connect();
    bool connectWithPassword();
    bool Authorize();
    bool insertCGAL(const std::string& key, const shared_ptr<const CGAL_Nef_polyhedron> &N);
    bool insertGeometry(const std::string& key, const shared_ptr<const Geometry> &geom);
    shared_ptr<const class CGAL_Nef_polyhedron> getCGAL(const std::string& key);
    shared_ptr<const class Geometry> getGeometry(const std::string& key);
    bool containsCGAL(const std::string& key);
    bool containsGeom(const std::string& key);
    bool insert(const std::string& key, const std::string& serializedgeom);
    bool get(const std::string& key, std::string& serializedgeom);
    bool contains(const std::string& key);
    const std::string getErrMsg(){ return err;}
    bool connectionStatus(){ return cstatus;}
    bool ping();
    bool checkReply(redisReply* reply);
    bool checkContext(redisContext* rct);
    bool clear();
    void print();

    static PCache* getInst(){if(!pCache) pCache = new PCache; return pCache;}

    void disconnect();
    virtual ~PCache() {}

    struct CGAL_cache_entry{
        std::string N;
        std::string msg;
        CGAL_cache_entry(std::string &N);
        CGAL_cache_entry() {}
        ~CGAL_cache_entry() {}
    };

    struct Geom_cache_entry{
        shared_ptr<const class Geometry> geom;
        std::string msg;
        Geom_cache_entry(const shared_ptr<const Geometry> &geom);
        Geom_cache_entry() {}
        ~Geom_cache_entry() { }
    };

private:
    redisContext* rct;
    redisReply* reply;
    bool cstatus;
    std::string host, pass;
    unsigned int port;
    std::string err;

    static PCache* pCache;

};
#else
class PCache
{
    PCache() {PRINT("Unable to detect Hiredis");}
};

#endif
