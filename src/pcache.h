#pragma once
#include <string>
#include "printutils.h"

#ifdef ENABLE_HIREDIS

#include <hiredis/hiredis.h>

class PCache
{
public:
    PCache();

    void init(const std::string _host, const uint16_t _port, const std::string _pass);
    bool connect();
    bool connectWithPassword();
    bool Authorize();
    bool insertCGAL(const std::string& key, const std::string& serializedgeom);
    bool insertGeometry(const std::string& key, const std::string& serializedgeom);
    bool getCGAL(const std::string& key, std::string& serializedgeom);
    bool getGeometry(const std::string& key, std::string& serializedgeom);
    bool containsCGAL(const std::string& key, bool& ret);
    bool containsGeom(const std::string& key, bool& ret);
    bool insert(const std::string& key, const std::string& serializedgeom);
    bool get(const std::string& key, std::string& serializedgeom);
    bool contains(const std::string& key, bool& ret);
    const std::string getErrMsg(){ return err;}
    bool connectionStatus(){ return cstatus;}
    bool ping();
    bool checkReply(redisReply* reply);
    bool checkContext(redisContext* rct);
    bool clear();
    void print();

    static PCache* getInst(){if(!pCache) pCache = new PCache; return pCache;}

    void disconnect();
    virtual ~PCache() {disconnect();}

private:
    redisContext* rct;
    redisReply* reply;
    bool cstatus;
    std::string host, pass;
    uint16_t port;
    std::string err;

    static PCache* pCache;

};
#else
class PCache
{
    PCache() {PRINT("Unable to detect Hiredis");}
};

#endif
