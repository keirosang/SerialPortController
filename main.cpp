#include "SerialPort.h"
#include "Config.h"
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
std::vector<std::chrono::system_clock::time_point> lastDataTimes;

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

void displayStatus(const std::vector<PortConfig>& configs) {
    while (true) {
        std::lock_guard<std::mutex> lock(console_mutex);
        clearConsole();
        gotoxy(0, 0);
        
        // 显示表头
        setTextColor(Color::White);
        std::cout << "Serial Port Monitor Status" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        std::cout << std::setw(5) << "No." 
                  << std::setw(10) << "Port" 
                  << std::setw(12) << "Baud Rate" 
                  << std::setw(15) << "Data Status" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        // 获取当前时间
        auto now = std::chrono::system_clock::now();

        // 显示每个串口的状态
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << std::setw(5) << i + 1
                      << std::setw(10) << configs[i].name
                      << std::setw(12) << configs[i].baudRate;

            // 计算无数据时间
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - lastDataTimes[i]).count();

            // 根据状态设置颜色
            if (*portDataFlags[i]) {
                setTextColor(Color::Green);
                std::cout << std::setw(15) << "Receiving";
            }
            else if (duration >= configs[i].timeout) {
                setTextColor(Color::Red);
                std::cout << std::setw(15) << "No Data";
            }
            else {
                setTextColor(Color::Yellow);
                std::cout << std::setw(15) << "Waiting";
            }
            
            setTextColor(Color::White);
            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void collectData(const PortConfig& config, size_t portIndex) {
    SerialPort port(config);
    if (!port.open()) {
        std::cerr << "Failed to open port: " << config.name << std::endl;
        return;
    }

    std::vector<char> buffer;
    while (true) {
        if (port.read(buffer) && !buffer.empty()) {
            if (isEmptyOrWhitespace(buffer)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // 更新数据标志和最后接收时间
            *portDataFlags[portIndex] = true;
            lastDataTimes[portIndex] = std::chrono::system_clock::now();
            
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            
            // 创建数据目录和保存数据
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

            // 重置数据标志（1秒后）
            std::this_thread::sleep_for(std::chrono::seconds(1));
            *portDataFlags[portIndex] = false;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    auto now = std::chrono::system_clock::now();
    
    for (size_t i = 0; i < configs.size(); ++i) {
        portDataFlags[i] = std::make_unique<bool>(false);
        lastDataTimes[i] = now;  // 初始化最后接收时间
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