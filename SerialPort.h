#pragma once
#include <string>
#include <vector>

struct PortConfig {
    std::string name;
    int baudRate;
    int dataBits;
    int stopBits;
    std::string parity;
    bool addTimestamp;
    int timeout;
};

class SerialPort {
public:
    SerialPort(const PortConfig& config);
    ~SerialPort();

    bool open();
    bool close();
    bool read(std::vector<char>& buffer);
    bool isOpen() const { return m_isOpen; }
    const PortConfig& getConfig() const { return m_config; }

private:
    PortConfig m_config;
    bool m_isOpen;
#ifdef _WIN32
    void* m_handle;  // HANDLE for Windows
#else
    int m_handle;    // File descriptor for Linux
#endif
}; 