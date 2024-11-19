cmake_minimum_required(VERSION 3.10)
project(SerialPortCollector)

# 设置编译器
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(nlohmann_json 3.2.0 REQUIRED)

# Add source files
set(SOURCES
    main.cpp
    SerialPort.cpp
    Config.cpp
    TcpClient.cpp
)

# Add header files
set(HEADERS
    SerialPort.h
    Config.h
    TcpClient.h
    Common.h
    Logger.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# 根据平台添加不同的链接选项
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm ws2_32)
endif()

if(MSVC)
    add_compile_options(/utf-8)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif() 