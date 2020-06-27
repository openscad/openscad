#include "PCSettings.h"

PCSettings *PCSettings::inst = nullptr;

PCSettings *PCSettings::instance()
{
    if(!inst){
        inst = new PCSettings();
    }
    return inst;
}

PCSettings::PCSettings()
{
    ipAddress = "127.0.0.1";
    port = 6379;
    enablePersistentCache = false;
    enableAuth = false;
}
