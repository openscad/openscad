#pragma once

#include <string>

class PCSettings{
public:
    static PCSettings* instance();
    std::string ipAddress, password;
    uint16_t port;
    bool enablePersistentCache;
private:
    static PCSettings* inst;
    PCSettings();
    ~PCSettings() {}
};
