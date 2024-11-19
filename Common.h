#pragma once
#include <string>

struct TcpConfig {
    bool enabled;
    std::string server;
    int port;
    int reconnectInterval;
}; 