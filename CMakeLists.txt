cmake_minimum_required(VERSION 3.10)
project(SerialPortCollector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(nlohmann_json 3.2.0 REQUIRED)

# Add source files
set(SOURCES
    main.cpp
    SerialPort.cpp
    Config.cpp
)

# Add header files
set(HEADERS
    SerialPort.h
    Config.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
endif() 