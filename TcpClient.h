#pragma once
#include "Common.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>

class TcpClient {
public:
    TcpClient(const TcpConfig& config);
    ~TcpClient();

    void start();
    void stop();
    bool send(const std::string& data);

private:
    void connectLoop();
    bool connect();
    void disconnect();
    void processQueue();
    void clearQueue();

    TcpConfig m_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;
    
#ifdef _WIN32
    unsigned long long m_socket;
#else
    int m_socket;
#endif

    std::thread m_connectThread;
    std::thread m_processThread;
    std::queue<std::string> m_dataQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
}; 