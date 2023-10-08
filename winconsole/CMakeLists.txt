cmake_minimum_required(VERSION 3.5...3.20)

project(winconsole VERSION 1.0 LANGUAGES C)

set(CMAKE_EXECUTABLE_SUFFIX_C .com)

add_executable(winconsole winconsole.c)

set_target_properties(winconsole PROPERTIES OUTPUT_NAME openscad${SUFFIX_WITH_DASH})

install(TARGETS winconsole RUNTIME DESTINATION ".")
