# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(unittests LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Test Widgets)

set(CMAKE_AUTOMOC ON)

enable_testing(true)

add_executable(unittests
    main.cpp
    TabManagerTest.cpp
    TabManagerTest.h
)

set_target_properties(unittests PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE)

target_link_libraries(unittests PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Test
    Qt5::Widgets
)

install(TARGETS unittests
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

