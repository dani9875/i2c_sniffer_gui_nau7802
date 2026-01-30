cmake_minimum_required(VERSION 3.16)
project(QtPythonViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(QtPythonViewer
    main.cpp
    mainwindow.h
    mainwindow.cpp
)

target_link_libraries(QtPythonViewer Qt6::Widgets)
