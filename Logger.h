#pragma once
#include <string>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void logError(const std::string& portName, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);

        std::filesystem::path dirPath = "error";
        std::filesystem::create_directories(dirPath);

        std::ostringstream filename;
        filename << std::put_time(&timeinfo, "%Y%m%d") << ".log";
        auto filepath = dirPath / filename.str();

        std::ofstream file(filepath, std::ios::app);
        if (file.is_open()) {
            file << "[" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "] "
                 << portName << ": " << message << std::endl;
        }
    }

private:
    Logger() = default;
    std::mutex m_mutex;
};

#define LOG_ERROR(port, msg) Logger::getInstance().logError(port, msg) 