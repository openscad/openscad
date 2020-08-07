#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <sstream>
#include "SCADSerializations.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include "pcache.h"
#include "printutils.h"

BOOST_CLASS_EXPORT(PolySet);
BOOST_CLASS_EXPORT(Polygon2d);

#ifdef ENABLE_HIREDIS

PCache *PCache::pCache = nullptr;

PCache::PCache(){
    host.clear();
    port = 6379;
    err.clear();
    rct = NULL;
    reply = NULL;
    cstatus = false;
}

void PCache::init(const std::string _host, const unsigned int _port, const std::string _pass){
    host = _host;
    port = _port;
    pass = _pass;
}

bool PCache::connect(){
    rct = redisConnect(host.c_str(), port);
    if(checkContext(rct)){
        return false;
    }else{
        if(!ping()){
            return false;
        }
    }
    return true;
}

bool PCache::connectWithPassword(){
    rct = redisConnect(host.c_str(), port);
    if(checkContext(rct)){
        return false;
    }else{
        if(!Authorize()){
            return false;
        }
    }
    return true;
}

bool PCache::Authorize(){
    reply= static_cast<redisReply*>(redisCommand(rct, "AUTH %s", pass.c_str()));
    if(checkReply(reply)){
        return false;
    }else return true;
}

void PCache::disconnect(){
    if(cstatus && rct != NULL){
        redisFree(rct);
        rct = NULL;
    }
    cstatus = false;
}

bool PCache::ping(){
    if(!cstatus || rct == NULL){
        return false;
    }
    reply = static_cast<redisReply*>(redisCommand(rct, "PING"));
    if(checkReply(reply)){
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool PCache::insert(const std::string& key, const std::string& serializedGeom){
    //PRINTDB("PCacheReader R: %s = %s", key.c_str() % serializedGeom.c_str() );
    reply = static_cast<redisReply*>(redisCommand(rct, "SET %s %s", key.c_str(), serializedGeom.c_str()));
    if(checkReply(reply)){
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool PCache::get(const std::string &key, std::string &serializedGeom){
    reply = static_cast<redisReply*>(redisCommand(rct, "GET %s", key.c_str()));
    if(checkReply(reply)){
        return false;
    }
    serializedGeom = reply->str;
    freeReplyObject(reply);
    return true;
}
// Insert methods takes in key and unserialized Geometry. need to change the function arguments.
// Serialize the geometry and insert that into redis using insert method
// Returns on operation success
bool PCache::insertCGAL(const std::string &key, const shared_ptr<const CGAL_Nef_polyhedron> &N){
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

bool PCache::insertGeometry(const std::string &key, const shared_ptr<const Geometry> &geom){
    PRINTDB("Insert: %s", key.c_str());
    std::stringstream ss;
    Geom_cache_entry ce(geom);
    boost::archive::text_oarchive oa(ss);
    oa << ce;
    std::string data = ss.str();
    return insert("GEOM-"+key, data);
}

shared_ptr<const CGAL_Nef_polyhedron> PCache::getCGAL(const std::string &key){
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

shared_ptr<const Geometry> PCache::getGeometry(const std::string &key){
    PRINTDB("Get: %s", key.c_str());
    std::string data;
    shared_ptr<const Geometry> geom;
    Geom_cache_entry ce;
    if(get("GEOM-"+key, data)){
        std::stringstream ss(data);
        boost::archive::text_iarchive io(ss);
        io >> ce;
        geom = ce.geom;
    }
    return geom;
}

bool PCache::containsCGAL(const std::string &key){
    PRINTDB("Contains: %s", key.c_str());
    return contains("CGAL-"+key);
}

bool PCache::containsGeom(const std::string &key){
    PRINTDB("Contains: %s", key.c_str());
    return contains("GEOM-"+key);
}

bool PCache::contains(const std::string &key){
    reply = static_cast<redisReply*>(redisCommand(rct, "EXISTS %s", key.c_str()));
    if(checkReply(reply)){
        freeReplyObject(reply);
        return false;
    }
    if(reply->integer==1){
        freeReplyObject(reply);
        return true;
    }else{
        freeReplyObject(reply);
        return false;
    }
}

bool PCache::flushall(){
    reply = static_cast<redisReply*>(redisCommand(rct, "FLUSHALL"));
    if(checkReply(reply)){
        return false;
    }
    freeReplyObject(reply);
    return true;
}

void PCache::print(){
    if(!cstatus){
        return;
    }
    reply = static_cast<redisReply*>(redisCommand(rct, "KEYS CGAL-*"));
    if(checkReply(reply)){
        return;
    }
    PRINTB("CGAL Polyhedrons in cache: %d", reply->elements);
    freeReplyObject(reply);
    reply = static_cast<redisReply*>(redisCommand(rct, "KEYS GEOM-*"));
    if(checkReply(reply)){
        return;
    }
    PRINTB("Geometries in cache: %d", reply->elements);
    freeReplyObject(reply);
}

bool PCache::checkContext(redisContext *rct){
    if(rct == NULL || rct->err){
        if(rct){
            err = rct->err;
            redisFree(rct);
        }else{
            err = "NULL Context";
        }
        cstatus = false;
        PRINTB("WARNING:(Persistent Cache) Unable to establish connection due to: %s", err);
        return true;
    }
    cstatus=true;
    return false;
}

bool PCache::checkReply(redisReply *reply){
    if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
        if(reply){
            err = reply->str;
            freeReplyObject(reply);
        }else{
            err = "NULL Reply";
        }
        PRINTB("WARNING:(Persistent Cache) Cannot cache operations due to: %s", err);
        return true;
    }
    return false;
}

#endif

CGAL_cache_entry::CGAL_cache_entry(std::string &N) : N(N){
    if (print_messages_stack.size() > 0) msg = print_messages_stack.back();
}

Geom_cache_entry::Geom_cache_entry(const shared_ptr<const Geometry> &geom) : geom(geom){
    if (print_messages_stack.size() > 0) msg = print_messages_stack.back();
}

