#include "SerialPort.h"
#include "Config.h"
#include "TcpClient.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <algorithm>
#include <vector>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif

#define _CRT_SECURE_NO_WARNINGS

std::mutex console_mutex;
std::vector<std::unique_ptr<bool>> portDataFlags;
std::vector<std::chrono::steady_clock::time_point> lastDataTimes;

// 颜色定义
enum class Color {
    Red = 12,
    Yellow = 14,
    Green = 10,
    White = 15
};

// 设置控制台文本颜色
void setTextColor(Color color) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
#else
    switch (color) {
        case Color::Red:    std::cout << "\033[31m"; break;
        case Color::Yellow: std::cout << "\033[33m"; break;
        case Color::Green:  std::cout << "\033[32m"; break;
        case Color::White:  std::cout << "\033[37m"; break;
    }
#endif
}

// 清除控制台
void clearConsole() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coordScreen = {0, 0};
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coordScreen, &cCharsWritten);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
    SetConsoleCursorPosition(hConsole, coordScreen);
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

// 移动光标到指定位置
void gotoxy(int x, int y) {
#ifdef _WIN32
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
#endif
}

std::string getDateString() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &time);
    
    std::ostringstream oss;
    oss << std::put_time(&timeinfo, "%Y%m%d");
    return oss.str();
}

bool isEmptyOrWhitespace(const std::vector<char>& buffer) {
    return std::all_of(buffer.begin(), buffer.end(), 
        [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

// 添加数据速率统计结构
struct PortStats {
    size_t bytesReceived;
    std::chrono::steady_clock::time_point lastUpdate;
    double bytesPerSecond;
    int packetsInLastSecond;
    std::chrono::steady_clock::time_point lastPacketTime;
    bool isActive;  // 添加活动状态标志
};

std::vector<PortStats> portStats;

void displayStatus(const std::vector<PortConfig>& configs) {
    // 初始化统计数据
    portStats.resize(configs.size());
    lastDataTimes.resize(configs.size());
    auto now = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < configs.size(); ++i) {
        portStats[i].bytesReceived = 0;
        portStats[i].lastUpdate = now;
        portStats[i].bytesPerSecond = 0.0;
        portStats[i].packetsInLastSecond = 0;
        portStats[i].lastPacketTime = now;
        portStats[i].isActive = false;
        lastDataTimes[i] = now;
    }

    while (true) {
        {
            std::lock_guard<std::mutex> lock(console_mutex);
            clearConsole();

            // 显示表头
            setTextColor(Color::White);
            std::cout << "Serial Port Collector v1.0.2" << std::endl;
            std::cout << std::string(50, '-') << std::endl;
            std::cout << std::setw(4) << "No."
                      << std::setw(8) << "Port"
                      << std::setw(10) << "Baud"
                      << std::setw(12) << "Status"
                      << std::setw(16) << "Speed(B/s)" << std::endl;
            std::cout << std::string(50, '-') << std::endl;

            // 显示每个串口的状态
            for (size_t i = 0; i < configs.size(); ++i) {
                std::cout << std::setw(4) << i + 1
                          << std::setw(8) << configs[i].name
                          << std::setw(10) << configs[i].baudRate;

                auto currentTime = std::chrono::steady_clock::now();
                auto timeSinceLastData = std::chrono::duration_cast<std::chrono::seconds>(
                    currentTime - lastDataTimes[i]).count();

                // 根据超时时间和活动状态判断显示状态
                if (timeSinceLastData >= configs[i].timeout) {
                    setTextColor(Color::Red);
                    std::cout << std::setw(12) << "Offline";
                    portStats[i].isActive = false;
                    portStats[i].bytesPerSecond = 0.0;  // 清零速率
                }
                else if (portStats[i].isActive) {
                    setTextColor(Color::Green);
                    std::cout << std::setw(12) << "Active";
                }
                else {
                    setTextColor(Color::Yellow);
                    std::cout << std::setw(12) << "Waiting";
                    portStats[i].bytesPerSecond = 0.0;  // 清零速率
                }

                setTextColor(Color::White);
                std::cout << std::setw(16) << std::fixed << std::setprecision(1) 
                          << portStats[i].bytesPerSecond << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 检查每个端口的数据接收情况
        for (size_t i = 0; i < configs.size(); ++i) {
            auto currentTime = std::chrono::steady_clock::now();
            auto timeSinceLastData = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - lastDataTimes[i]).count();
            
            // 如果超过1秒没有新数据，清零速率
            if (timeSinceLastData > 1) {
                portStats[i].bytesPerSecond = 0.0;
            }
        }
    }
}

void saveToFile(const PortConfig& config, const std::vector<char>& buffer) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::filesystem::path dirPath = "data";
    dirPath /= config.name;
    std::filesystem::create_directories(dirPath);

    std::string filename = getDateString() + ".data";
    auto filepath = dirPath / filename;

    std::ofstream file(filepath, std::ios::app);
    
    if (config.addTimestamp) {
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        file << std::put_time(&timeinfo, "[%Y-%m-%d %H:%M:%S] ");
    }
    
    file.write(buffer.data(), buffer.size());
    if (buffer.back() != '\n') {
        file << std::endl;
    }
}

void collectData(const PortConfig& config, size_t portIndex) {
    SerialPort port(config);
    if (!port.open()) {
        LOG_ERROR(config.name, "Failed to open port");
        return;
    }

    // 创建 TCP 客户端
    TcpClient tcpClient(config.tcpForward);
    if (config.tcpForward.enabled) {
        tcpClient.start();
        LOG_ERROR(config.name, "TCP forwarding enabled -> " + 
                  config.tcpForward.server + ":" + 
                  std::to_string(config.tcpForward.port));
    }

    std::vector<char> buffer;
    bool lastReadFailed = false;
    
    while (true) {
        bool readResult = port.read(buffer);
        if (readResult && !buffer.empty()) {
            if (isEmptyOrWhitespace(buffer)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // 更新数据包统计和状态
            portStats[portIndex].packetsInLastSecond++;
            portStats[portIndex].isActive = true;  // 设置活动状态
            lastDataTimes[portIndex] = std::chrono::steady_clock::now();
            
            // 保存到文件
            saveToFile(config, buffer);

            // TCP 转发
            if (config.tcpForward.enabled) {
                std::string data(buffer.begin(), buffer.end());
                tcpClient.send(data);
            }

            // 更新数据速率统计
            portStats[portIndex].bytesReceived += buffer.size();
            auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - portStats[portIndex].lastUpdate).count();
            
            if (timeDiff >= 1) {
                portStats[portIndex].bytesPerSecond = 
                    static_cast<double>(portStats[portIndex].bytesReceived) / timeDiff;
                portStats[portIndex].bytesReceived = 0;
                portStats[portIndex].lastUpdate = std::chrono::steady_clock::now();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            if (!readResult && !lastReadFailed) {
                LOG_ERROR(config.name, "Read failed - Further errors will be suppressed");
                lastReadFailed = true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cout.tie(nullptr);

    std::vector<PortConfig> configs;
    if (!Config::load("config.json", configs)) {
        return 1;
    }

    if (configs.empty()) {
        std::cerr << "No ports configured in config file." << std::endl;
        return 1;
    }

    // 初始化端口数据标志和最后接收时间
    portDataFlags.resize(configs.size());
    lastDataTimes.resize(configs.size());
    auto currentTime = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < configs.size(); ++i) {
        portDataFlags[i] = std::make_unique<bool>(false);
        lastDataTimes[i] = currentTime;  // 使用 steady_clock 的时间点
    }

    // 创建状态显示线程
    std::thread statusThread(displayStatus, std::ref(configs));

    // 创建数据采集线程
    std::vector<std::thread> threads;
    for (size_t i = 0; i < configs.size(); ++i) {
        threads.emplace_back(collectData, std::ref(configs[i]), i);
    }

    // 等待线程结束
    statusThread.join();
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
} 