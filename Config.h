#pragma once
#include "SerialPort.h"
#include <vector>
#include <string>

class Config {
public:
    static bool load(const std::string& filename, std::vector<PortConfig>& configs);
    static bool saveDefault(const std::string& filename);

private:
    static std::vector<PortConfig> getDefaultConfig();
}; 