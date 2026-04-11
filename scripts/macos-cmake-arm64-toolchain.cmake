# CMake toolchain file for building arm64 macOS dependencies.
#
# When cmake itself is an x86_64 binary (e.g. from MacPorts running under
# Rosetta 2), it auto-discovers MacPorts x86_64 libraries and headers via
# platform search paths. This toolchain file overrides that behaviour by
# directing cmake to search the macOS SDK (for system libraries) and the
# custom DEPLOYDIR (for pre-built dependencies) before falling back to
# host paths. Both search roots are included so that custom-built libs in
# DEPLOYDIR and SDK system libs are both found correctly.
#
# Usage (set DEPLOYDIR and MACOS_SDK in the environment before calling cmake):
#   cmake -DCMAKE_TOOLCHAIN_FILE=<this_file> ...

cmake_minimum_required(VERSION 3.13)

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Prefer SDK and DEPLOYDIR over host prefix paths (e.g. /opt/local from MacPorts).
# BOTH mode: cmake searches root paths first, then falls back to host paths.
if(DEFINED ENV{MACOS_SDK} AND DEFINED ENV{DEPLOYDIR})
  set(CMAKE_FIND_ROOT_PATH "$ENV{DEPLOYDIR};$ENV{MACOS_SDK}" CACHE STRING "cmake find root")
elseif(DEFINED ENV{DEPLOYDIR})
  set(CMAKE_FIND_ROOT_PATH "$ENV{DEPLOYDIR}" CACHE STRING "cmake find root")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH CACHE STRING "")
