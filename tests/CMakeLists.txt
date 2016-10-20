# instructions - see ../doc/testing.txt

cmake_minimum_required(VERSION 2.8.3)
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.3)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()

project(tests)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(TESTDATA_DIR "${CMAKE_SOURCE_DIR}/../testdata")
set(TESTTOOLS_DIR "${tests_SOURCE_DIR}")
set(MCAD_LIBRARIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libraries")
set(EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/../examples")

# Find GUI openscad binary (typically built with qmake)
if(EXISTS "$ENV{OPENSCAD_BINARY}")
  set(OPENSCAD_BINPATH "$ENV{OPENSCAD_BINARY}")
elseif(APPLE)
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../OpenSCAD.app/Contents/MacOS/OpenSCAD")
elseif (MINGW_CROSS_ENV_DIR) 
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../mingw32/release/openscad.exe")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Release/openscad.exe")
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../Release/openscad.exe")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../bin/openscad.exe")
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../bin/openscad.exe")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../bin/openscad")
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../bin/openscad")
else()
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../openscad")
endif()

if(EXISTS "${OPENSCAD_BINPATH}")
  message(STATUS "Found OpenSCAD binary: ${OPENSCAD_BINPATH}")
else()
  message(STATUS "Couldn't find the OpenSCAD binary: ${OPENSCAD_BINPATH}")
  message(FATAL_ERROR "Please build the OpenSCAD binary and place it here: ${OPENSCAD_BINPATH}" )
endif()


# Search for MCAD in correct place
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/MCAD/__init__.py)
  message(FATAL_ERROR "MCAD not found. You can install from the OpenSCAD root as follows: \n  git submodule update --init")
endif()
set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};OPENSCADPATH=${MCAD_LIBRARIES_DIR}")

include("CMakeCompilerSettings.cmake")

# Find image comparison program

find_package(ImageMagick COMPONENTS convert)
if (ImageMagick_convert_FOUND)
  message(STATUS "ImageMagick convert executable found: " ${ImageMagick_convert_EXECUTABLE})
  set(IMAGE_COMPARE_EXECUTABLE ${ImageMagick_convert_EXECUTABLE})
else()
  message(STATUS "Couldn't find imagemagick 'convert' program")
  set(DIFFPNG 1)
endif()

if (ImageMagick_convert_FOUND)
  execute_process(COMMAND ${IMAGE_COMPARE_EXECUTABLE} --version OUTPUT_VARIABLE IM_OUT )
  if ( ${IM_OUT} MATCHES "OpenMP" )
    # http://www.daniloaz.com/en/617/systems/high-cpu-load-when-converting-images-with-imagemagick
    message(STATUS "ImageMagick: OpenMP bug workaround - setting MAGICK_THREAD_LIMIT=1")
    set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};MAGICK_THREAD_LIMIT=1")
  endif()

  message(STATUS "Comparing magicktest1.png with magicktest2.png")
  set(IM_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/magicktest1.png" "${CMAKE_CURRENT_SOURCE_DIR}/magicktest2.png")
  set(COMPARE_ARGS ${IMAGE_COMPARE_EXECUTABLE} ${IM_TEST_FILES} -alpha Off -compose difference -composite -threshold 10% -morphology Erode Square -format %[fx:w*h*mean] info:)
  # compare arguments taken from test_cmdline_tool.py
  message(STATUS "Running ImageMagick compare: ${COMPARE_ARGS}")
  execute_process(COMMAND ${COMPARE_ARGS} RESULT_VARIABLE IM_RESULT OUTPUT_VARIABLE IM_OUT )
  message(STATUS "Result: ${IM_RESULT}")
  if ( NOT ${IM_RESULT} STREQUAL "0" )
    message(STATUS "magicktest1.png and magicktest2.png were incorrectly detected as identical")
    message(STATUS "Using alternative image comparison")
    set(DIFFPNG 1)
 endif()
endif()

if ( ${DIFFPNG} )
  set(IMAGE_COMPARE_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/diffpng)
  set(COMPARATOR "diffpng")
  add_executable(diffpng diffpng.cpp ../src/lodepng.cpp)
  message(STATUS "using diffpng for image comparison")
endif()


# Re-distributable MSYS ctest package
# no executables, just a portable ctest script
if (MSYS_DISTRIB)
  set(TESTS_DEPLOY_DIR "C:/Program Files/OpenSCAD/tests")
  message(STATUS "build msys2 test distribution package. copying testdata...")

  # copy testdata so paths setup by cmake will work on any user machine. 
  # use 'cp' shell command because file(COPY) bugs with msys2+cmake+unicode
  set(CMDL1 "#!/usr/bin/bash\n")
  set(CMDL2 "cp -av " ${TESTDATA_DIR} " " ${TESTS_DEPLOY_DIR}/.. "\n")
  set(CMDL3 "cp -av " ${CMAKE_SOURCE_DIR}/regression " " ${TESTS_DEPLOY_DIR} "\n")
  set(CMD ${CMDL1} ${CMDL2} ${CMDL3})
  string(REPLACE "Program Files" "Program\\ Files" CMD ${CMD})
  file(WRITE "${TESTS_DEPLOY_DIR}/wincopy.sh" ${CMD})
  execute_process(COMMAND sh wincopy.sh OUTPUT_VARIABLE result ERROR_VARIABLE result)
  message(STATUS ${result})

  file(GLOB PYFILES ${CMAKE_SOURCE_DIR}/*.py)
  file(GLOB CMAKEFILES ${CMAKE_SOURCE_DIR}/Enforce*.cmake)
  file(GLOB PNGFILES ${CMAKE_SOURCE_DIR}/*.png)
  file(GLOB BATFILES ${CMAKE_SOURCE_DIR}/*.bat)
  file(GLOB MISCFILES ${CMAKE_SOURCE_DIR}/cgalsanitytest)
  file(COPY ${PYFILES} ${PNGFILES} ${CMAKEFILES} ${MISCFILES} ${CCFILES} ${BATFILES} DESTINATION ${TESTS_DEPLOY_DIR})

  set(TESTDATA_DIR ${TESTS_DEPLOY_DIR}/../testdata)
  set(ENV{OPENSCAD_BINARY} ${TESTS_DEPLOY_DIR}/../openscad.exe)
  set(TESTTOOLS_DIR ${TESTS_DEPLOY_DIR})
  set(MCAD_LIBRARIES_DIR ${TESTS_DEPLOY_DIR}/../libraries)
  set(EXAMPLES_DIR ${TESTS_DEPLOY_DIR}/../examples)
  set(PYTHON_EXECUTABLE "C:/Python27/python.exe")
#  message(FATAL_ERROR "hi")
endif()

if (MSYS)
  # software rendering of gl on windows machines
  message(STATUS "Detecting software GL driver for Windows(TM)")
  set(GLFILENAME "C:/msys64/mingw64/bin/opengl32.dll")
  if(EXISTS ${GLFILENAME})
    get_filename_component(OPENSCAD_BINDIR ${OPENSCAD_BINPATH} DIRECTORY)
    message(STATUS "copying ${GLFILENAME} to ${OPENSCAD_BINDIR}")
    file(COPY ${GLFILENAME} DESTINATION ${OPENSCAD_BINDIR})
  else()
    message(STATUS "could not find opengl32.dll")
  endif()
endif()

# Find dependency libraries and tools

include("CMakeDependencies.cmake")

# Build Test executables

include("CMakeExecutables.cmake")

# Build CTestTestfile.cmake (the test scripts)

include("CMakeTests.cmake")

#message("Available test configurations: ${TEST_CONFIGS}")
#foreach(CONF ${TEST_CONFIGS})
#  message("${CONF}: ${${CONF}_TEST_CONFIG}")
#endforeach()

message(STATUS "CPPFLAGS: ${CMAKE_CXX_FLAGS}")
