#pragma once

#include <string>
#include <stdint.h>

class PCSettings{
public:
    static PCSettings* instance();
    std::string ipAddress, password;
    uint port;
    bool enablePersistentCache, enableAuth;
private:
    static PCSettings* inst;
    PCSettings();
    ~PCSettings() {}
};
