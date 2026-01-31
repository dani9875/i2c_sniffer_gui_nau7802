cmake_minimum_required(VERSION 3.16)
project(WeighSniffer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(WeighSniffer
    main.cpp
    mainwindow.h
    mainwindow.cpp
)

target_link_libraries(WeighSniffer Qt6::Widgets)

# --- Copy test1.py to the output directory of the executable ---
add_custom_command(TARGET WeighSniffer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/test1.py"
        "$<TARGET_FILE_DIR:WeighSniffer>/test1.py"
    COMMENT "Copying test1.py next to executable"
)