# Serial Port Data Collector

A cross-platform serial port data collection tool that supports multiple ports simultaneously.

## Features

- Multi-port concurrent data collection
- Configurable port parameters via JSON
- Real-time status display with color indicators
- Automatic data file management
- Cross-platform support (Windows/Linux)

## Directory Structure

```bash
SerialPortCollector/
├── CPP/
│   ├── main.cpp          # Main program entry
│   ├── SerialPort.h      # Serial port class declaration
│   ├── SerialPort.cpp    # Serial port implementation
│   ├── Config.h          # Configuration class declaration
│   ├── Config.cpp        # Configuration implementation
│   └── CMakeLists.txt    # CMake build configuration
├── data/                 # Data storage directory (auto-created)
└── config.json          # Port configuration file
```

## File Description

### main.cpp
- Program entry point
- Multi-threaded data collection
- Status monitoring and display
- Data storage management

### SerialPort.h/cpp
- Serial port communication implementation
- Cross-platform serial operations
- Windows and Linux support

### Config.h/cpp
- Configuration file management
- JSON configuration parsing
- Default configuration generation

### CMakeLists.txt
- CMake project configuration
- Cross-platform build support
- Dependency management

## Detailed Code Documentation

### Main Classes and Structures

#### PortConfig Structure
```cpp
struct PortConfig {
    std::string name;        // Port name
    int baudRate;           // Baud rate
    int dataBits;          // Data bits
    int stopBits;          // Stop bits
    std::string parity;    // Parity mode
    bool addTimestamp;     // Enable timestamp
    int timeout;           // Timeout in seconds
};
```

#### SerialPort Class
- Handles port open, close, and read operations
- Cross-platform implementation
- Error handling and status management

#### Config Class
- Configuration file reading and parsing
- Default configuration generation
- Configuration validation and error handling

## Development Environment

### Windows Environment
- Visual Studio 2019/2022
- CMake 3.10+
- vcpkg package manager
- Windows SDK 10.0+

### Linux Environment
- GCC 8.0+
- CMake 3.10+
- Serial port development library

### Dependencies
- nlohmann-json: JSON parsing library

## Build Steps

### Windows Platform

1. Install required tools:
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

2. Install dependencies
```bash
.\vcpkg install nlohmann-json:x64-windows
```
2. Build project:
```bash
cd SerialPortCollector/CPP
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Linux Platform

1. Install dependencies:
```bash
sudo apt-get install build-essential cmake
sudo apt-get install nlohmann-json3-dev
```

2. Build project:
```bash
    cd SerialPortCollector/CPP
    mkdir build
    cd build
    cmake ..
    make
```

## Configuration

### config.json Example
```json    
{
    "ports": [
        {
            "name": "COM1",
            "baudRate": 9600,
            "dataBits": 8,
            "stopBits": 1,
            "parity": "none",
            "addTimestamp": true,
            "timeout": 60
        }
    ]
}
```

### Parameter Description
- name: Port name (Windows: COM1, Linux: /dev/ttyUSB0)
- baudRate: Baud rate (common values: 9600, 115200)
- dataBits: Data bits (typically 8)
- stopBits: Stop bits (1 or 2)
- parity: Parity check ("none", "odd", "even")
- addTimestamp: Enable timestamp in data
- timeout: Data timeout threshold in seconds

## Runtime Status Display

### Status Color Indicators
- Green: Actively receiving data
- Yellow: Waiting for data
- Red: No data received (timeout exceeded)

### Display Content
- Port number
- Port name
- Baud rate
- Current status

## Data Storage Format

### Directory Structure
```bash
data/
├── COM1/
│   ├── 20240118.data
│   └── 20240119.data
└── COM2/
    ├── 20240118.data
    └── 20240119.data
```

### Data File Format
- Filename: YYYYMMDD.data
- Data format: [timestamp] data content (if timestamp enabled)

## Troubleshooting

### Common Issues
1. Port Open Failure
   - Verify port name
   - Check if port is in use
   - Verify user permissions

2. Data Reception Issues
   - Check baud rate settings
   - Verify port parameters
   - Check hardware connection

3. Build Errors
   - Verify all dependencies are installed
   - Check CMake configuration
   - Verify compiler version

## License

MIT License

## Author

Luo Qi
- GitHub: [@keirosang](https://github.com/keirosang)

## Auto-start Configuration

### Windows System

1. Create Shortcut
   - Locate the compiled program (SerialPortCollector.exe)
   - Right-click to create shortcut
   - Right-click the shortcut, select "Properties"
   - Fill in the "Start in" field with the full path to the program directory
   - Or modify "Target" by adding `cd /d` command, for example:
     ```
     cmd /c "cd /d C:\Program Files\SerialPortCollector && SerialPortCollector.exe"
     ```

2. Add to Startup Folder (ensure correct working directory)
   - Press Win + R, type `shell:startup` to open startup folder
   - Copy the configured shortcut to the startup folder

3. Use Task Scheduler (Recommended)
   - Open Task Scheduler (search for "Task Scheduler")
   - Create Basic Task
   - Set name and description (e.g., "SerialPortCollector")
   - Choose trigger "When computer starts"
   - Action select "Start a program"
   - Browse to select program path
   - **Important:** Fill in "Start in (optional)" with the full path to program directory
   - Optional: Set "Run with highest privileges"

### Linux System

1. Use Systemd Service (Recommended)
   - Create service file:
```bash
sudo nano /etc/systemd/system/serialportcollector.service
```
   - Add following content:
```ini
[Unit]
Description=Serial Port Collector Service
After=network.target

[Service]
Type=simple
User=your_username
WorkingDirectory=/path/to/program/directory
ExecStart=/path/to/program/SerialPortCollector
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```
   - Enable service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable serialportcollector
sudo systemctl start serialportcollector
```

2. Use rc.local (for older systems)
   - Edit rc.local file:
```bash
sudo nano /etc/rc.local
```
   - Add before `exit 0`:
```bash
/path/to/program/SerialPortCollector &
```
   - Ensure rc.local is executable:
```bash
sudo chmod +x /etc/rc.local
```

3. Use Desktop Environment Autostart (GUI environment)
   - Create .desktop file:
```bash
nano ~/.config/autostart/serialportcollector.desktop
```
   - Add following content:
```ini
[Desktop Entry]
Type=Application
Name=SerialPortCollector
Exec=/path/to/program/SerialPortCollector
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
```

### Important Notes

1. Windows Notes:
   - Ensure program path contains no Chinese characters
   - Recommended to run with administrator privileges
   - Can set restart on failure in Task Scheduler
   - **Working Directory Setup:**
     * Program needs correct working directory to find config file
     * Use one of these methods to ensure correct working directory:
       1. Set "Start in" in shortcut properties
       2. Use batch file to start program:
          ```batch
          @echo off
          cd /d "%~dp0"
          start SerialPortCollector.exe
          ```
       3. Modify program code to use program directory as config path
     * Recommended to keep config file in same directory as program

2. Linux Notes:
   - Ensure program has execution permissions
   - Check user permissions for serial devices
   - View systemd service logs:
```bash
sudo systemctl status serialportcollector
sudo journalctl -u serialportcollector
```

3. General Notes:
   - Use absolute paths when possible
   - Ensure correct config file location
   - Consider adding error logging
   - Ensure data directory has write permissions