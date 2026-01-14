cmake_minimum_required(VERSION 3.16)
project(FTDI_Viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# THIS fixes the vtable error
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFTDI1 REQUIRED libftdi1)

add_executable(FTDI_Viewer
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_include_directories(FTDI_Viewer PRIVATE
    ${LIBFTDI1_INCLUDE_DIRS}
)

target_link_libraries(FTDI_Viewer PRIVATE
    Qt6::Widgets
    ${LIBFTDI1_LIBRARIES}
)
