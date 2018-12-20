cmake_minimum_required(VERSION 3.0)

project(libidf-main)
include_directories(.)

find_package(Boost 1.62 REQUIRED COMPONENTS program_options)

add_executable(libidf-main libidf-main.cc idf3.cc)
target_link_libraries(libidf-main Boost::program_options)
