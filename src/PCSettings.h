#pragma once

#include <string>

class PCSettings{
public:
    static PCSettings* instance();
    std::string ipAddress, password;
    uint16_t port;
private:
    static PCSettings* inst;
    PCSettings();
    ~PCSettings() {}
};
