cmake_minimum_required(VERSION 3.16)
project(FTDI_Viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# THIS fixes the vtable error
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

if (WIN32)
    # On Windows, libftdi is expected to come from vcpkg
    find_package(libusb-1.0 REQUIRED)
    find_package(libftdi1 CONFIG REQUIRED)

    get_target_property(_libusb_targets libusb-1.0::libusb-1.0 INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "libusb include dirs: ${_libusb_targets}")

    set(LIBFTDI1_LIBRARIES libftdi1 libusb-1.0)
    # include dirs are handled by the imported targets
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBFTDI1 REQUIRED libftdi1)
endif()

add_executable(FTDI_Viewer
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

if (NOT WIN32)
    target_include_directories(FTDI_Viewer PRIVATE
        ${LIBFTDI1_INCLUDE_DIRS}
    )
endif()

target_link_libraries(FTDI_Viewer PRIVATE
    Qt6::Widgets
    ${LIBFTDI1_LIBRARIES}
)
