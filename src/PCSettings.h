#pragma once

#include <string>

class PCSettings{
public:
    static PCSettings* instance();
    std::string ipAddress, password;
    unsigned int port;
    bool enablePersistentCache, enableAuth;
private:
    static PCSettings* inst;
    PCSettings();
    ~PCSettings() {}
};
