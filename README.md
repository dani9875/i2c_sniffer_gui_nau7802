# i2c_sniffer_gui_nau7802

## Overview
This project reads I2C sniffer data via an FT232 USB-to-Serial module and displays it in a GUI.

## Hardware Requirements
- FT232 USB-to-Serial module (FT232R / FT232RL)
- I2C sniffer hardware connected to the FT232
- USB cable

## Software Requirements
- Windows 10 or newer
- libusb
- Zadig (https://zadig.akeo.ie/)

## Windows Setup

### 1. Connect the FT232
1. Plug the FT232 module into your PC
2. Open Device Manager
3. Verify the device appears as one of:
   - USB Serial Converter
   - FT232R USB UART

The device may initially appear under "Ports (COM & LPT)".

### 2. Install WinUSB Driver Using Zadig
1. Download and run Zadig as Administrator
2. In Zadig, enable:
   Options -> List All Devices
3. Select your FT232 device from the device list
4. Select "WinUSB" as the target driver
5. Click "Replace Driver"
6. Wait until Zadig reports success

### 3. Verify Driver Installation
1. Open Device Manager
2. The FT232 should now appear under:
   Universal Serial Bus devices

Note:
After switching to WinUSB, the FT232 will no longer appear as a COM port.
This is expected. Communication is handled via libusb.

## Application Notes
- The application communicates with the FT232 using libusb
- Serial terminal programs (PuTTY, Tera Term, etc.) will not work
- Only one application can access the device at a time

## Troubleshooting

### FT232 Not Visible in Zadig
- Enable "Options -> List All Devices"
- Unplug and reconnect the FT232
- Try a different USB port
- Confirm the device appears in Device Manager

### Restore Default FTDI Driver
To restore normal COM port functionality:
1. Open Device Manager
2. Right-click the FT232 device
3. Select "Uninstall device"
4. Enable "Delete the driver software for this device"
5. Unplug and reconnect the FT232

Windows will automatically reinstall the default FTDI driver.

If Windows does not automatically reinstall the driver, download and install the
FTDI Virtual COM Port (VCP) driver manually from:
https://ftdichip.com/drivers/vcp-drivers/



## Linux Setup

To build the project on Linux (Ubuntu/Debian-based), you need to install Qt, libusb, and libftdi development packages (or you can use local libraries).

### 1. Update package list
```
sudo apt update
```
### 2. Update package list
```
sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools
```

### 3. Update package list
```
sudo apt install libusb-1.0-0-dev
```

### 4. Update package list
```
sudo apt install libftdi1-dev
```

### 5. Verify installation
```
pkg-config --modversion libusb-1.0
pkg-config --modversion libftdi1
```



## Windows Build Instructions

### Prerequisites
- Windows 10 or newer
- Qt (MinGW build)
- MSYS2
- CMake
- MinGW toolchain
---

### 1. Install MSYS2 and Required Tools
1. Install MSYS2 from: https://www.msys2.org/
2. Open the **MSYS2 MinGW64** shell
3. Install required packages: 
```
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake make
```
4. Create a symlink so `make` calls `mingw32-make`:
```
ln -s /mingw64/bin/mingw32-make.exe /mingw64/bin/make
```

### 2. Install Qt (MinGW Build)

1. Install Qt using the Qt Online Installer  
   - You may need to create a Qt account to download the installer.
2. Select **Qt 6.10.1 → MinGW 64-bit** during installation.
3. Example installation path:
C:\Qt\6.10.1\mingw_64
> Qt must be explicitly configured in CMake when using the MinGW build.

Add the following to your `CMakeLists.txt`:

```cmake
# Qt (MinGW build)
set(Qt6_DIR "C:/Qt/6.10.1/mingw_64/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
```

### 3. Configuring and building the project (from root folder)
```
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

### External Libraries and DLLs

- **Qt DLLs**  
  - Version: Qt 6.10.1  
  - All required Qt DLLs are included in the project build.

- **libftdi**  
  - MinGW-compatible libftdi binaries are included in the project.  
  - Originally obtained from:  
    [https://sourceforge.net/projects/picusb/files/libftdi1-1.5_devkit_x86_x64_19July2020.zip/download](https://sourceforge.net/projects/picusb/files/libftdi1-1.5_devkit_x86_x64_19July2020.zip/download)  
    > Note: The link may be obsolete

- **libusb**  
  - MinGW-compatible libusb binaries are included in the project.  
  - Originally obtained from:  
    [https://github.com/libusb/libusb/releases/download/v1.0.29/libusb-1.0.29.7z](https://github.com/libusb/libusb/releases/download/v1.0.29/libusb-1.0.29.7z)  
    > Note: The link may be obsolete








## Linux Build Instructions

```
mkdir build
cd build
cmake ..
make
```