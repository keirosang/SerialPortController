#include "Config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

bool Config::load(const std::string& filename, std::vector<PortConfig>& configs) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return saveDefault(filename);
    }

    try {
        json j;
        file >> j;

        configs.clear();
        for (const auto& port : j["ports"]) {
            PortConfig config;
            config.name = port["name"].get<std::string>();
            config.baudRate = port["baudRate"].get<int>();
            config.dataBits = port["dataBits"].get<int>();
            config.stopBits = port["stopBits"].get<int>();
            config.parity = port["parity"].get<std::string>();
            config.addTimestamp = port["addTimestamp"].get<bool>();
            config.timeout = port.value("timeout", 60);
            config.tcpForward.enabled = port.value("tcpForward", json::object())
                .value("enabled", false);
            config.tcpForward.server = port.value("tcpForward", json::object())
                .value("server", "127.0.0.1");
            config.tcpForward.port = port.value("tcpForward", json::object())
                .value("port", 8080);
            config.tcpForward.reconnectInterval = port.value("tcpForward", json::object())
                .value("reconnectInterval", 5);
            configs.push_back(config);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::vector<PortConfig> Config::getDefaultConfig() {
    std::vector<PortConfig> configs;
    PortConfig config;
    config.name = "COM1";
    config.baudRate = 9600;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = "none";
    config.addTimestamp = true;
    config.timeout = 60;
    configs.push_back(config);
    return configs;
}

bool Config::saveDefault(const std::string& filename) {
    auto configs = getDefaultConfig();
    
    json j;
    j["ports"] = json::array();
    
    for (const auto& config : configs) {
        json port;
        port["name"] = config.name;
        port["baudRate"] = config.baudRate;
        port["dataBits"] = config.dataBits;
        port["stopBits"] = config.stopBits;
        port["parity"] = config.parity;
        port["addTimestamp"] = config.addTimestamp;
        port["timeout"] = config.timeout;
        j["ports"].push_back(port);
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(4);
    std::cout << "Created default config file: " << filename << std::endl;
    std::cout << "Please modify the config file and run the program again." << std::endl;
    return true;
} 