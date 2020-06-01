#include "pcache.h"
#include "printutils.h"

#ifdef ENABLE_HIREDIS

PCache::PCache(){
    host.clear();
    port = 6379;
    err.clear();
    rct = NULL;
    reply = NULL;
    cstatus = false;
}

void PCache::init(const std::string _host, const uint16_t _port, const std::string _pass){
    host = _host;
    port = _port;
    pass = _pass;
}

void PCache::connect(){
    rct = redisConnect(host.c_str(), port);
    if(checkContext(rct)){
        //do something to communicate the err to user and store cache in an usual way.
    }else{
        if(!ping()){
            //do something to communicate the err to user and store cache in an usual way.
        }
    }

}

void PCache::connectWithPassword(){
    rct = redisConnect(host.c_str(), port);
    if(checkContext(rct)){
        //do something to communicate the err to user and store cache in an usual way.
    }else{
        if(!Authorize()){
            //do something to communicate the err to user and store cache in an usual way.
        }
    }
}

bool PCache::Authorize(){
    reply= static_cast<redisReply*>(redisCommand(rct, "AUTH %s", pass.c_str()));
    if(checkReply(reply)){
        return false; //problem
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
    reply = static_cast<redisReply*>(redisCommand(rct, "SET %s %s", key.c_str(), serializedGeom.c_str()));
    if(checkReply(reply)){
        //do something to communicate this to the user and store cache in an usual way.
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool PCache::get(const std::string &key, std::string &serializedGeom){
    reply = static_cast<redisReply*>(redisCommand(rct, "GET %s", key.c_str()));
    if(checkReply(reply)){
        //do something to communicate this to the user and store cache in an usual way.
        return false;
    }
    serializedGeom = reply->str;
    freeReplyObject(reply);
    return true;
}

bool PCache::insertCGAL(const std::string &key, const std::string &serializedgeom){
    return insert("CGAL-"+key, serializedgeom);
}

bool PCache::insertGeometry(const std::string &key, const std::string &serializedgeom){
    return insert("GEOM-"+key, serializedgeom);
}

bool PCache::getCGAL(const std::string &key, std::string &serializedgeom){
    return get("CGAL-"+key, serializedgeom);
}

bool PCache::getGeometry(const std::string &key, std::string &serializedgeom){
    return get("GEOM-"+key, serializedgeom);
}

bool PCache::contains(const std::string &key, bool &ret){
    reply = static_cast<redisReply*>(redisCommand(rct, "KEYS %s", key.c_str()));
    if(checkReply(reply)){
        //do something to communicate this to the user and store cache in an usual way.
        return false;
    }
    if(reply->elements==1){
        ret = true;
    }else{
        ret = false;
    }
    freeReplyObject(reply);
    return true;
}

bool PCache::clear(){
    reply = static_cast<redisReply*>(redisCommand(rct, "FLUSHALL"));
    if(checkReply(reply)){
        //do something to communicate this to the user and store cache in an usual way.
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
        return true;
    }
    return false;
}

#endif
