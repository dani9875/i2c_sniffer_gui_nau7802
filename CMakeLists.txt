cmake_minimum_required(VERSION 3.16)
project(FTDI_Viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Qt (Windows: MinGW) - use environment variable if set
# ---------------------------------------------------------
if(WIN32 AND NOT DEFINED ENV{QT6_DIR})
    message(FATAL_ERROR "Please set the QT6_DIR environment variable or install Qt")
endif()

if(DEFINED ENV{QT6_DIR})
    set(Qt6_DIR $ENV{QT6_DIR})
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------
# Executable
# ---------------------------------------------------------
add_executable(FTDI_Viewer
    main.cpp
    mainwindow.cpp
    ftdireader.cpp
)

# ---------------------------------------------------------
# Handle libraries differently by platform
# ---------------------------------------------------------
if(WIN32)
    # Windows: use bundled libusb and libftdi
    set(LIBUSB_ROOT  "${CMAKE_CURRENT_SOURCE_DIR}/libusb")
    set(LIBFTDI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libftdi")

    target_include_directories(FTDI_Viewer PRIVATE
        "${LIBUSB_ROOT}/include"
        "${LIBFTDI_ROOT}/include"
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Link order matters for .a (libftdi depends on libusb)
    target_link_libraries(FTDI_Viewer PRIVATE
        Qt6::Widgets
        "${LIBFTDI_ROOT}/libftdi1.a"
        "${LIBUSB_ROOT}/libusb-1.0.a"
    )
else()
    # Linux: find installed packages
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    pkg_check_modules(LIBFTDI REQUIRED libftdi1)

    target_include_directories(FTDI_Viewer PRIVATE
        ${LIBUSB_INCLUDE_DIRS}
        ${LIBFTDI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(FTDI_Viewer PRIVATE
        Qt6::Widgets
        ${LIBFTDI_LIBRARIES}
        ${LIBUSB_LIBRARIES}
    )
endif()

# ---------------------------------------------------------
# Copy runtime DLLs next to the executable after build (Windows only)
# ---------------------------------------------------------
if(WIN32)
    set(DLL_DIR "${CMAKE_SOURCE_DIR}/dll")

    add_custom_command(TARGET FTDI_Viewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${DLL_DIR}"
                "$<TARGET_FILE_DIR:FTDI_Viewer>"
        COMMENT "Copying runtime DLLs to output directory"
    )
endif()
