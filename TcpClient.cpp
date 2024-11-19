#include "TcpClient.h"
#include "Logger.h"
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (-1)
#endif

TcpClient::TcpClient(const TcpConfig& config)
    : m_config(config), m_running(false), m_connected(false), m_socket(INVALID_SOCKET) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

TcpClient::~TcpClient() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void TcpClient::start() {
    m_running = true;
    m_connectThread = std::thread(&TcpClient::connectLoop, this);
    m_processThread = std::thread(&TcpClient::processQueue, this);
}

void TcpClient::stop() {
    m_running = false;
    m_queueCV.notify_all();
    
    if (m_connectThread.joinable()) {
        m_connectThread.join();
    }
    if (m_processThread.joinable()) {
        m_processThread.join();
    }
    
    disconnect();
}

bool TcpClient::send(const std::string& data) {
    if (!m_config.enabled) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_dataQueue.push(data);
    m_queueCV.notify_one();
    return true;
}

void TcpClient::clearQueue() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::queue<std::string> empty;
    std::swap(m_dataQueue, empty);
}

void TcpClient::connectLoop() {
    while (m_running) {
        if (!m_connected) {
            if (connect()) {
                m_connected = true;
            } else {
                std::this_thread::sleep_for(
                    std::chrono::seconds(m_config.reconnectInterval));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool TcpClient::connect() {
    disconnect();

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        return false;
    }

#ifdef _WIN32
    // 设置发送超时
    DWORD timeout = 2000; // 2秒
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 2;  // 2秒
    timeout.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_config.port);
    inet_pton(AF_INET, m_config.server.c_str(), &serverAddr.sin_addr);

    if (::connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        disconnect();
        return false;
    }

    return true;
}

void TcpClient::disconnect() {
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
    m_connected = false;
    clearQueue();  // 断开连接时清空队列
}

void TcpClient::processQueue() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_queueCV.wait(lock, [this] { 
            return !m_running || !m_dataQueue.empty(); 
        });

        while (!m_dataQueue.empty() && m_connected) {
            std::string data = m_dataQueue.front();
            m_dataQueue.pop();
            lock.unlock();

            if (m_socket != INVALID_SOCKET) {
                if (data.length() > static_cast<size_t>(INT_MAX)) {
                    LOG_ERROR("TCP", "Data size exceeds maximum send limit");
                    disconnect();
                    break;
                }
                int result = ::send(m_socket, data.c_str(), static_cast<int>(data.length()), 0);
                if (result == SOCKET_ERROR) {
                    LOG_ERROR("TCP", "Send timeout or error, clearing queue");
                    disconnect();  // 这会清空队列
                    break;  // 跳出发送循环
                }
            }

            lock.lock();
        }
    }
} 