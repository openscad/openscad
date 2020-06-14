#pragma once

#include <string>

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
