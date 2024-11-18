#include "SerialPort.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

SerialPort::SerialPort(const PortConfig& config) 
    : m_config(config), m_isOpen(false), m_handle(nullptr) {}

SerialPort::~SerialPort() {
    if (m_isOpen) {
        close();
    }
}

bool SerialPort::open() {
#ifdef _WIN32
    std::string portName = "\\\\.\\" + m_config.name;
    m_handle = CreateFileA(portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (m_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(m_handle, &dcb)) {
        CloseHandle(m_handle);
        return false;
    }

    dcb.BaudRate = m_config.baudRate;
    dcb.ByteSize = m_config.dataBits;
    dcb.StopBits = m_config.stopBits == 1 ? ONESTOPBIT : TWOSTOPBITS;
    dcb.Parity = m_config.parity == "none" ? NOPARITY :
                 m_config.parity == "odd" ? ODDPARITY : EVENPARITY;

    if (!SetCommState(m_handle, &dcb)) {
        CloseHandle(m_handle);
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    SetCommTimeouts(m_handle, &timeouts);

#else
    m_handle = ::open(m_config.name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (m_handle < 0) {
        return false;
    }

    struct termios options;
    tcgetattr(m_handle, &options);
    
    // Set baud rate
    speed_t baud;
    switch (m_config.baudRate) {
        case 9600: baud = B9600; break;
        case 115200: baud = B115200; break;
        default: baud = B9600;
    }
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      // 8 data bits

    tcsetattr(m_handle, TCSANOW, &options);
#endif

    m_isOpen = true;
    return true;
}

bool SerialPort::close() {
    if (!m_isOpen) return true;

#ifdef _WIN32
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
#else
    if (m_handle >= 0) {
        ::close(m_handle);
        m_handle = -1;
    }
#endif

    m_isOpen = false;
    return true;
}

bool SerialPort::read(std::vector<char>& buffer) {
    buffer.resize(1024);
    DWORD bytesRead = 0;

#ifdef _WIN32
    if (!ReadFile(m_handle, buffer.data(), buffer.size(), &bytesRead, NULL)) {
        return false;
    }
#else
    bytesRead = ::read(m_handle, buffer.data(), buffer.size());
    if (bytesRead < 0) {
        return false;
    }
#endif

    buffer.resize(bytesRead);
    return bytesRead > 0;
} 