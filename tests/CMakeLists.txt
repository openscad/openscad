# instructions - see ../doc/testing.txt

#set(DEBUG_OSCD 1) # print debug info during cmake

cmake_minimum_required(VERSION 2.8)
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.3)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()

# Needed for cmake < 2.8.3
include(CMakeParseArguments.cmake)

# Detect Lion and force gcc
IF (APPLE)
   EXECUTE_PROCESS(COMMAND sw_vers -productVersion OUTPUT_VARIABLE MACOSX_VERSION)
   IF (NOT ${MACOSX_VERSION} VERSION_LESS "10.9.0")
     message("Detected Maverick (10.9) or later")
     set(CMAKE_C_COMPILER "clang")
     set(CMAKE_CXX_COMPILER "clang++")
     # Somehow, since we build dependencies for 10.7, we need to also build executables
     # for 10.7. This used to not be necessary, but since 10.9 it apparently is..
     SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.7 CACHE STRING "Deployment target")
   ELSEIF (NOT ${MACOSX_VERSION} VERSION_LESS "10.8.0")
     message("Detected Mountain Lion (10.8)")
     set(CMAKE_C_COMPILER "clang")
     set(CMAKE_CXX_COMPILER "clang++")
   ELSEIF (NOT ${MACOSX_VERSION} VERSION_LESS "10.7.0")
     message("Detected Lion (10.7)")
     set(CMAKE_C_COMPILER "clang")
     set(CMAKE_CXX_COMPILER "clang++")
   ELSE()
     message("Detected Snow Leopard (10.6) or older")
     if (USE_LLVM)
       message("Using LLVM compiler")
       set(CMAKE_C_COMPILER "llvm-gcc")
       set(CMAKE_CXX_COMPILER "llvm-g++")
     endif()
   ENDIF()
ENDIF(APPLE)


project(tests)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

# Build debug build as default
if(NOT CMAKE_BUILD_TYPE)
  #  set(CMAKE_BUILD_TYPE Release)
  if(CMAKE_COMPILER_IS_GNUCXX)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_GREATER 4.6)
      set(CMAKE_BUILD_TYPE RelWithDebInfo)
    else()
      set(CMAKE_BUILD_TYPE Debug)
    endif()
  else()
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

# MCAD
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/MCAD/__init__.py)
  message(FATAL_ERROR "MCAD not found. You can install from the OpenSCAD root as follows: \n  git submodule update --init")
endif()

# NULLGL - Allow us to build without OpenGL(TM). run 'cmake .. -DNULLGL=1'
# Most tests will fail, but it can be used for testing/experiments

if(NULLGL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNULLGL")
  set(SKIP_IMAGEMAGICK "1") # we dont generate png, so nothing to compare
else()
  set(ENABLE_OPENCSG_FLAG "-DENABLE_OPENCSG")
endif()

#
# Windows
#

if(WIN32 AND MSVC)
  set(WIN32_STATIC_BUILD "True")
endif()

if(WIN32_STATIC_BUILD AND MSVC)
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(EMSG "\nTo build Win32 STATIC OpenSCAD please see doc/testing.txt")
    message(FATAL_ERROR ${EMSG})
  endif()
endif()

# Disable warnings
if(WIN32 AND MSVC)
  # too long decorated names
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503")
  # int cast to bool in CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800")
  # unreferenced parameters in CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100")
  # fopen_s advertisement
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_DEPRECATE")
  # lexer uses strdup & other POSIX stuff
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_NONSTDC_NO_DEPRECATE")
  # M_PI
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")
endif()

# Debugging - if you uncomment, use nmake -f Makefile > log.txt (the log is big)
if(WIN32 AND MSVC)
  # Linker debugging
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -VERBOSE")

  # Compiler debugging
  # you have to pass -DCMAKE_VERBOSE_MAKEFILE=ON to cmake when you run it. 
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  if (WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -frounding-math")
  endif()
endif()

# Clang compiler

if(${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
  # disable enormous amount of warnings about CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

#
# Build test apps
#

function(inclusion user_set_path found_paths)
  # Set up compiler include paths with prepend/append rules. Input is 
  # a path and a set of paths. If user_set_path matches anything in found_paths
  # then we prepend the found_paths because we assume the user wants
  # their set_paths to be a priority. 

  if (DEBUG_OSCD)
    message(STATUS "inclusion:")
    message(STATUS "  ${user_set_path}: ${${user_set_path}}")
    message(STATUS "  ${found_paths}: ${${found_paths}}")
  endif()
  set(inclusion_match 0)
  if (${user_set_path})
    foreach(found_path ${${found_paths}})
      string(FIND ${found_path} ${${user_set_path}} INDEX)
      if (DEFINED INDEX)
        if (${INDEX} EQUAL 0)
          set(inclusion_match 1)
        endif()
      endif()
    endforeach()
    if (inclusion_match)
      include_directories(BEFORE ${${found_paths}})
      if (DEBUG_OSCD)
        message(STATUS "inclusion prepend ${${found_paths}} for ${user_set_path}")
      endif()
      set(inclusion_match 0)
    endif()
  endif()
  if (NOT inclusion_match)
    include_directories(AFTER ${${found_paths}})
    if (DEBUG_OSCD)
      message(STATUS "inclusion append ${${found_paths}} for ${user_set_path}")
    endif()
  endif()
endfunction()

# Boost
# 
# FindBoost.cmake has been included from Cmake's GIT circa the end of 2011
# because most existing installs of cmake had a buggy older version. 
#
# Update this if FindBoost.cmake gets out of sync with the current boost release
# set(Boost_ADDITIONAL_VERSIONS "1.47.0" "1.46.0")

if (WIN32)
  set(Boost_USE_STATIC_LIBS TRUE)
  set(BOOST_STATIC TRUE)
  set(BOOST_THREAD_USE_LIB TRUE)
endif()

if (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  set(BOOST_ROOT "$ENV{OPENSCAD_LIBRARIES}")
  if (EXISTS ${BOOST_ROOT}/include/boost)
    # if boost is under OPENSCAD_LIBRARIES, then 
    # don't look in the system paths (workaround FindBoost.cmake bug)
    set(Boost_NO_SYSTEM_PATHS "TRUE")
    message(STATUS "BOOST_ROOT: " ${BOOST_ROOT})
  endif()
endif()

if (NOT $ENV{BOOSTDIR} STREQUAL "")
  set(BOOST_ROOT "$ENV{BOOSTDIR}")
  set(Boost_NO_SYSTEM_PATHS "TRUE")
  set(Boost_DEBUG TRUE)
  message(STATUS "BOOST_ROOT: " ${BOOST_ROOT})
endif()

find_package( Boost 1.35.0 COMPONENTS thread program_options filesystem system regex REQUIRED)
message(STATUS "Boost ${Boost_VERSION} includes found: " ${Boost_INCLUDE_DIRS})
message(STATUS "Boost libraries found:")
foreach(boostlib ${Boost_LIBRARIES})
  message(STATUS "  " ${boostlib})
endforeach()

inclusion(BOOST_ROOT Boost_INCLUDE_DIRS)
if (${Boost_VERSION} LESS 104600)
  add_definitions(-DBOOST_FILESYSTEM_VERSION=3) # Use V3 for boost 1.44-1.45
endif()

# On Mac, we need to link against the correct C++ library. We choose the same one
# as Boost uses.
if(APPLE)
  execute_process(COMMAND grep -q __112basic_string ${Boost_LIBRARIES}
                  RESULT_VARIABLE BOOST_USE_STDLIBCPP)
  if (NOT BOOST_USE_STDLIBCPP)
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

# Mac OS X
if(APPLE)
  FIND_LIBRARY(COCOA_LIBRARY Cocoa REQUIRED)
  FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices)
endif()


# Eigen

# Turn off Eigen SIMD optimization
if(NOT APPLE)
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "^FreeBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEIGEN_DONT_ALIGN")
  endif()
endif()

# Priority
# 3. EIGENDIR if set
# 1. OPENSCAD_LIBRARIES eigen3
# 4. system's standard include paths for eigen3

set(EIGEN_DIR "$ENV{EIGENDIR}")
set(OPENSCAD_LIBDIR "$ENV{OPENSCAD_LIBRARIES}")

if (EIGEN_DIR)
  set(EIGHINT ${EIGEN_DIR}/include/eigen3 ${EIGEN_DIR})
  find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS ${EIGHINT})
endif()

if (NOT EIGEN_INCLUDE_DIR)
  # The NO_CMAKE_SYSTEM_PATH is necessary to avoid OS X from picking up 
  # a stray Eigen from /Library/Framework
  find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS ${OPENSCAD_LIBDIR}/include/eigen3 NO_CMAKE_SYSTEM_PATH)
endif()

if (NOT EIGEN_INCLUDE_DIR)
  if (${CMAKE_SYSTEM_NAME} MATCHES "^FreeBSD")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/local/include/eigen3)
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/pkg/include/eigen3)
  elseif (APPLE)
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /opt/local/include/eigen3)
  else()
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/include/eigen3)
  endif()
endif()

if (NOT EIGEN_INCLUDE_DIR)
  message(STATUS "Eigen not found")
else()
  message(STATUS "Eigen found in " ${EIGEN_INCLUDE_DIR})
  inclusion(EIGEN_DIR EIGEN_INCLUDE_DIR)
endif()

###### NULLGL wraps all OpenGL(TM) items (GL, Glew, OpenCSG)
###### Several pages of code fall under this 'if( NOT NULLGL )'
if (NOT NULLGL)

# OpenGL
find_package(OpenGL REQUIRED)
if (NOT OPENGL_GLU_FOUND)
  message(STATUS "GLU not found in system paths...searching $ENV{OPENSCAD_LIBRARIES} ")
  find_library(OPENGL_glu_LIBRARY GLU HINTS $ENV{OPENSCAD_LIBRARIES}/lib)
  if (NOT OPENGL_glu_LIBRARY)
    message(FATAL "GLU library not found")
  endif()
  set(OPENGL_LIBRARIES ${OPENGL_glu_LIBRARY} ${OPENGL_LIBRARIES})
  message(STATUS "OpenGL LIBRARIES: ")
  foreach(GLLIB ${OPENGL_LIBRARIES})
    message(STATUS "  " ${GLLIB})
  endforeach()
endif()

# OpenCSG
if (NOT $ENV{OPENCSGDIR} STREQUAL "")
  set(OPENCSG_DIR "$ENV{OPENCSGDIR}")
elseif (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  set(OPENCSG_DIR "$ENV{OPENSCAD_LIBRARIES}")
endif()
if (NOT OPENCSG_INCLUDE_DIR)
  message(STATUS "OPENCSG_DIR: " ${OPENCSG_DIR})
  find_path(OPENCSG_INCLUDE_DIR
            opencsg.h
            HINTS ${OPENCSG_DIR}/include)
  find_library(OPENCSG_LIBRARY
               opencsg
               HINTS ${OPENCSG_DIR}/lib)
  if (NOT OPENCSG_INCLUDE_DIR OR NOT OPENCSG_LIBRARY)
    message(FATAL_ERROR "OpenCSG not found")
  else()
    message(STATUS "OpenCSG include found in " ${OPENCSG_INCLUDE_DIR})
    message(STATUS "OpenCSG library found in " ${OPENCSG_LIBRARY})
  endif()
endif()
inclusion(OPENCSG_DIR OPENCSG_INCLUDE_DIR)

# GLEW

if(WIN32_STATIC_BUILD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLEW_STATIC")
endif()

if (NOT $ENV{GLEWDIR} STREQUAL "")
  set(GLEW_DIR "$ENV{GLEWDIR}")
elseif (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  set(GLEW_DIR "$ENV{OPENSCAD_LIBRARIES}")
endif()
if (GLEW_DIR)
  find_path(GLEW_INCLUDE_DIR
            GL/glew.h
            HINTS ${GLEW_DIR}/include
	    NO_DEFAULT_PATH)
  find_library(GLEW_LIBRARY
               NAMES GLEW glew
               HINTS ${GLEW_DIR}/lib ${GLEW_DIR}/lib64
	       NO_DEFAULT_PATH)
  if (GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
    set(GLEW_FOUND 1)
  endif()
endif()

if (NOT GLEW_FOUND)
  find_package(GLEW REQUIRED)
endif()

message(STATUS "GLEW include: " ${GLEW_INCLUDE_DIR})
message(STATUS "GLEW library: " ${GLEW_LIBRARY})

inclusion(GLEW_DIR GLEW_INCLUDE_DIR)

endif() ########## NULLGL ENDIF

# Flex/Bison
find_package(BISON REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "^FreeBSD")
  # FreeBSD has an old flex in /usr/bin and a new flex in /usr/local/bin
  set(FLEX_EXECUTABLE /usr/local/bin/flex)
endif()

# prepend the dir where deps were built
if (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  set(OSCAD_DEPS "")
  set(OSCAD_DEPS_PATHS $ENV{OPENSCAD_LIBRARIES}/include)
  inclusion(OSCAD_DEPS OSCAD_DEPS_PATHS)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
  include_directories( /usr/pkg/include /usr/X11R7/include )
  set(FLEX_EXECUTABLE /usr/pkg/bin/flex)
  if(NOT ${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()
endif()

find_package(FLEX REQUIRED)
# The COMPILE_FLAGS and forced C++ compiler is just to be compatible with qmake
if (WIN32)
  set(FLEX_UNISTD_FLAG "-DYY_NO_UNISTD_H")
endif()
FLEX_TARGET(OpenSCADlexer ../src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Plexer ${FLEX_UNISTD_FLAG}")
BISON_TARGET(OpenSCADparser ../src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c COMPILE_FLAGS "-p parser")
ADD_FLEX_BISON_DEPENDENCY(OpenSCADlexer OpenSCADparser)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c PROPERTIES LANGUAGE "CXX")

# CGAL

# Disable rounding math check to allow usage of Valgrind
# This is needed as Valgrind currently does not correctly
# handle rounding modes used by CGAL.
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCGAL_DISABLE_ROUNDING_MATH_CHECK=ON")

if (NOT $ENV{CGALDIR} STREQUAL "")
  set(CGAL_DIR "$ENV{CGALDIR}")
elseif (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  if (EXISTS "$ENV{OPENSCAD_LIBRARIES}/lib/CGAL")
    set(CGAL_DIR "$ENV{OPENSCAD_LIBRARIES}/lib/CGAL")
    set(CMAKE_MODULE_PATH "${CGAL_DIR}" ${CMAKE_MODULE_PATH})
  elseif (EXISTS "$ENV{OPENSCAD_LIBRARIES}/include/CGAL")
    set(CGAL_DIR "$ENV{OPENSCAD_LIBRARIES}")
    set(CMAKE_MODULE_PATH "${CGAL_DIR}" ${CMAKE_MODULE_PATH})
  endif()
endif()
message(STATUS "CGAL_DIR: " ${CGAL_DIR})
find_package(CGAL REQUIRED)
message(STATUS "CGAL config found in " ${CGAL_USE_FILE} )
foreach(cgal_incdir ${CGAL_INCLUDE_DIRS})
  message(STATUS "CGAL include found in " ${cgal_incdir} )
endforeach()
message(STATUS "CGAL libraries found in " ${CGAL_LIBRARIES_DIR} )
if("${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}" VERSION_LESS 3.6)
  message(FATAL_ERROR "CGAL >= 3.6 required")
endif()
inclusion(CGAL_DIR CGAL_INCLUDE_DIRS)

#Remove bad BOOST libraries from CGAL 3rd party dependencies when they don't exist (such as on 64-bit Ubuntu 13.10).
#Libs of concern are /usr/lib/libboost_thread.so;/usr/lib/libboost_system.so;
#Confirmed bug in CGAL @ https://bugs.launchpad.net/ubuntu/+source/cgal/+bug/1242111
string(FIND "${CGAL_3RD_PARTY_LIBRARIES}" "/usr/lib/libboost_system.so" FIND_POSITION  )
if(NOT "-1" STREQUAL ${FIND_POSITION} )
  if(NOT EXISTS "/usr/lib/libboost_system.so")
    MESSAGE( STATUS "CGAL_3RD_PARTY_LIBRARIES:Removing non-existent /usr/lib/libboost_system.so" )
    string(REPLACE "/usr/lib/libboost_system.so" "" CGAL_3RD_PARTY_LIBRARIES ${CGAL_3RD_PARTY_LIBRARIES})
  endif()
endif() 
string(FIND "${CGAL_3RD_PARTY_LIBRARIES}" "/usr/lib/libboost_thread.so" FIND_POSITION  )
if(NOT "-1" STREQUAL ${FIND_POSITION} )
  if(NOT EXISTS "/usr/lib/libboost_thread.so")
    MESSAGE( STATUS "CGAL_3RD_PARTY_LIBRARIES:Removing non-existent /usr/lib/libboost_thread.so" )
    string(REPLACE "/usr/lib/libboost_thread.so" "" CGAL_3RD_PARTY_LIBRARIES ${CGAL_3RD_PARTY_LIBRARIES})
  endif()
endif() 

if (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
  foreach(CGAL3RDPLIB ${CGAL_3RD_PARTY_LIBRARIES})
    if(NOT EXISTS "${CGAL3RDPLIB}")
      MESSAGE( STATUS " Removing non-existent ${CGAL3RDPLIB}" )
      string(REPLACE "${CGAL3RDPLIB}" "" CGAL_3RD_PARTY_LIBRARIES ${CGAL_3RD_PARTY_LIBRARIES})
    endif()
  endforeach()
endif()

MESSAGE(STATUS "CGAL 3RD PARTY LIBS:")
foreach(CGAL3RDPLIB ${CGAL_3RD_PARTY_LIBRARIES})
  MESSAGE(STATUS " ${CGAL3RDPLIB}" )
endforeach()

if(${CMAKE_CXX_COMPILER} MATCHES ".*clang.*" AND NOT ${CGAL_CXX_FLAGS_INIT} STREQUAL "" )
	string(REPLACE "-frounding-math" "" CGAL_CXX_FLAGS_INIT ${CGAL_CXX_FLAGS_INIT})
	string(REPLACE "--param=ssp-buffer-size=4" "" CGAL_CXX_FLAGS_INIT ${CGAL_CXX_FLAGS_INIT})
endif()

# GLib2

find_package(GLIB2 2.2.0 REQUIRED)
add_definitions(${GLIB2_DEFINITIONS})
inclusion(GLIB2_DIR GLIB2_INCLUDE_DIRS)

# find libraries using pkg-config
find_package(PkgConfig REQUIRED)
include(PkgConfigTools.cmake)
save_pkg_config_env()

if (DEFINED ENV{OPENSCAD_LIBRARIES})
  set(ENV{PKG_CONFIG_PATH} "$ENV{OPENSCAD_LIBRARIES}/lib/pkgconfig")
endif()

pkg_check_modules(FONTCONFIG REQUIRED fontconfig>=2.8.0)
if (FONTCONFIG_VERSION)
  message(STATUS "fontconfig ${FONTCONFIG_VERSION} found: ${FONTCONFIG_INCLUDE_DIRS}")
endif()

pkg_check_modules(FREETYPE REQUIRED freetype2>=2.4.9)
if (FREETYPE_VERSION)
  message(STATUS "freetype2 ${FREETYPE_VERSION} found: ${FREETYPE_INCLUDE_DIRS}")
endif()

pkg_check_modules(HARFBUZZ REQUIRED harfbuzz>=0.9.19)
if (HARFBUZZ_VERSION)
  message(STATUS "harfbuzz ${HARFBUZZ_VERSION} found: ${HARFBUZZ_INCLUDE_DIRS}")
endif()

restore_pkg_config_env()

add_definitions(${FONTCONFIG_CFLAGS})
add_definitions(${FREETYPE_CFLAGS})
add_definitions(${HARFBUZZ_CFLAGS})

# Image comparison - expected test image vs actual generated image

if (DIFFPNG)
  # How to set cflags to optimize the executable?
  set(IMAGE_COMPARE_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/diffpng)
  set(COMPARATOR "diffpng")
  add_executable(diffpng diffpng.cpp ../src/lodepng.cpp)
  set(SKIP_IMAGEMAGICK 1)
  message(STATUS "using diffpng for image comparison")
endif()

if (SKIP_IMAGEMAGICK)
  if (NOT DIFFPNG)
    # cross-building depends on this
    set(IMAGE_COMPARE_EXECUTABLE "/bin/echo")
  endif()
else()
  find_package(ImageMagick COMPONENTS convert)
  if (ImageMagick_convert_FOUND)
    message(STATUS "ImageMagick convert executable found: " ${ImageMagick_convert_EXECUTABLE})
    set(IMAGE_COMPARE_EXECUTABLE ${ImageMagick_convert_EXECUTABLE})
  else()
    message(STATUS "Couldn't find imagemagick 'convert' program")
    set(DIFFPNG 1)
  endif()
  if ( "${ImageMagick_VERSION_STRING}" VERSION_LESS "6.5.9.4" )
    message(STATUS "ImageMagick version less than 6.5.9.4, cannot use -morphology comparison")
    message(STATUS "ImageMagick Using older image comparison method")
    set(COMPARATOR "old")
  endif()

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

# Internal includes
include_directories(../src ../src/libtess2/Include)

# Handle OpenSCAD version based on VERSION env. variable.
# Use current timestamp if not specified (development builds)
if ("$ENV{VERSION}" STREQUAL "")
  # Timestamp is only available in cmake >= 2.8.11  
  if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.10)
    string(TIMESTAMP VERSION "%Y.%m.%d")
  else()
    set(VERSION "2013.06")
  endif()
else()
  set(VERSION $ENV{VERSION})
endif()
message(STATUS "OpenSCAD version: ${VERSION}")
string(REGEX MATCHALL "^[0-9]+|[0-9]+|[0-9]+$" MYLIST "${VERSION}")
list(GET MYLIST 0 OPENSCAD_YEAR)
list(GET MYLIST 1 OPENSCAD_MONTH)
math(EXPR OPENSCAD_MONTH ${OPENSCAD_MONTH}) # get rid of leading zero
list(LENGTH MYLIST VERSIONLEN)
if (${VERSIONLEN} EQUAL 3)
  list(GET MYLIST 2 OPENSCAD_DAY)
  math(EXPR OPENSCAD_DAY ${OPENSCAD_DAY}) # get rid of leading zero
endif()

add_definitions(-DOPENSCAD_VERSION=${VERSION} -DOPENSCAD_YEAR=${OPENSCAD_YEAR} -DOPENSCAD_MONTH=${OPENSCAD_MONTH})
if (DEFINED OPENSCAD_DAY)
  add_definitions(-DOPENSCAD_DAY=${OPENSCAD_DAY})
endif()

add_definitions(-DOPENSCAD_TESTING -DENABLE_EXPERIMENTAL)

# Search for MCAD in correct place
set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};OPENSCADPATH=${CMAKE_CURRENT_SOURCE_DIR}/../libraries")

# Platform specific settings

if(APPLE)
	message(STATUS "Offscreen OpenGL Context - using Apple CGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextCGL.mm" CACHE TYPE STRING)
	set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-macosx.cc" CACHE TYPE STRING)
	set(PLATFORMUTILS_SOURCE "PlatformUtils-mac.mm" CACHE TYPE STRING)
elseif(UNIX)
	message(STATUS "Offscreen OpenGL Context - using Unix GLX")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextGLX.cc" CACHE TYPE STRING)
	set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-lodepng.cc" CACHE TYPE STRING)
	set(PLATFORMUTILS_SOURCE "PlatformUtils-posix.cc" CACHE TYPE STRING)
elseif(WIN32)
	message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextWGL.cc" CACHE TYPE STRING)
	set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-lodepng.cc" CACHE TYPE STRING)
	set(PLATFORMUTILS_SOURCE "PlatformUtils-win.cc" CACHE TYPE STRING)
endif()

set(CORE_SOURCES
  tests-common.cc 
  ../src/parsersettings.cc
  ../src/mathc99.cc
  ../src/linalg.cc
  ../src/colormap.cc
  ../src/Camera.cc
  ../src/handle_dep.cc 
  ../src/value.cc 
  ../src/calc.cc 
  ../src/grid.cc 
  ../src/expr.cc 
  ../src/func.cc 
  ../src/stackcheck.cc 
  ../src/localscope.cc 
  ../src/module.cc 
  ../src/ModuleCache.cc 
  ../src/node.cc 
  ../src/context.cc 
  ../src/modcontext.cc 
  ../src/evalcontext.cc 
  ../src/feature.cc
  ../src/csgterm.cc 
  ../src/csgtermnormalizer.cc 
  ../src/Geometry.cc 
  ../src/Polygon2d.cc 
  ../src/csgops.cc 
  ../src/transform.cc 
  ../src/color.cc 
  ../src/primitives.cc 
  ../src/projection.cc 
  ../src/cgaladv.cc 
  ../src/surface.cc 
  ../src/control.cc 
  ../src/render.cc 
  ../src/rendersettings.cc 
  ../src/dxfdata.cc 
  ../src/dxfdim.cc 
  ../src/offset.cc 
  ../src/linearextrude.cc 
  ../src/rotateextrude.cc 
  ../src/text.cc 
  ../src/printutils.cc 
  ../src/fileutils.cc 
  ../src/progress.cc 
  ../src/boost-utils.cc 
  ../src/FontCache.cc
  ../src/DrawingCallback.cc
  ../src/FreetypeRenderer.cc
  ../src/lodepng.cpp
  ../src/PlatformUtils.cc 
  ../src/${PLATFORMUTILS_SOURCE}
  ${FLEX_OpenSCADlexer_OUTPUTS}
  ${BISON_OpenSCADparser_OUTPUTS})

set(NOCGAL_SOURCES
  ../src/builtin.cc 
  ../src/import.cc
  ../src/export.cc
  ../src/LibraryInfo.cc
  ../src/polyset.cc
  ../src/polyset-utils.cc
  ../src/GeometryUtils.cc)


set(CGAL_SOURCES
  ${NOCGAL_SOURCES}
  ../src/CSGTermEvaluator.cc 
  ../src/CGAL_Nef_polyhedron.cc 
  ../src/cgalutils.cc 
  ../src/cgalutils-tess.cc 
  ../src/cgalutils-polyhedron.cc 
  ../src/CGALCache.cc
  ../src/CGAL_Nef_polyhedron_DxfData.cc
  ../src/Polygon2d-CGAL.cc
  ../src/svg.cc
  ../src/GeometryEvaluator.cc)

set(COMMON_SOURCES
  ../src/nodedumper.cc 
  ../src/traverser.cc 
  ../src/GeometryCache.cc 
  ../src/clipper-utils.cc 
  ../src/Tree.cc
  ../src/polyclipping/clipper.cpp
  ../src/libtess2/Source/bucketalloc.c
  ../src/libtess2/Source/dict.c
  ../src/libtess2/Source/geom.c
  ../src/libtess2/Source/mesh.c
  ../src/libtess2/Source/priorityq.c
  ../src/libtess2/Source/sweep.c
  ../src/libtess2/Source/tess.c)

#
# Offscreen OpenGL context source code
#

set(OFFSCREEN_SOURCES
  ../src/GLView.cc
  ../src/OffscreenView.cc
  ../src/${OFFSCREEN_CTX_SOURCE}
  ../src/${OFFSCREEN_IMGUTILS_SOURCE}
  ../src/imageutils.cc
  ../src/fbo.cc
  ../src/system-gl.cc
  ../src/export_png.cc
  ../src/CGALRenderer.cc
  ../src/ThrownTogetherRenderer.cc
  ../src/renderer.cc
  ../src/render.cc
  ../src/OpenCSGRenderer.cc
)

if(NULLGL)
  message(STATUS "NULLGL is set. Overriding previous OpenGL(TM) settings")
  set(OFFSCREEN_SOURCES
    ../src/NULLGL.cc # contains several 'nullified' versions of above .cc files
    ../src/OffscreenView.cc
    ../src/OffscreenContextNULL.cc
    ../src/export_png.cc
    ../src/${OFFSCREEN_IMGUTILS_SOURCE}
    ../src/imageutils.cc
    ../src/renderer.cc
    ../src/render.cc)
endif()

add_library(tests-core STATIC ${CORE_SOURCES})
target_link_libraries(tests-core ${OPENGL_LIBRARIES} ${GLIB2_LIBRARIES} ${FONTCONFIG_LDFLAGS} ${FREETYPE_LDFLAGS} ${HARFBUZZ_LDFLAGS} ${Boost_LIBRARIES} ${COCOA_LIBRARY})

add_library(tests-common STATIC ${COMMON_SOURCES})
target_link_libraries(tests-common tests-core)

add_library(tests-cgal STATIC ${CGAL_SOURCES})
set_target_properties(tests-cgal PROPERTIES COMPILE_FLAGS "${ENABLE_OPENCSG_FLAG} -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(tests-cgal tests-common ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${GMP_LIBRARIES} ${MPFR_LIBRARIES})

#
# Create non-CGAL tests
#
add_library(tests-nocgal STATIC ${NOCGAL_SOURCES})
target_link_libraries(tests-nocgal tests-common)
if (NOT NULLGL)
  set_target_properties(tests-nocgal PROPERTIES COMPILE_FLAGS "${ENABLE_OPENCSG_FLAG}")
endif()

add_library(tests-offscreen STATIC ${OFFSCREEN_SOURCES})
set_target_properties(tests-offscreen PROPERTIES COMPILE_FLAGS "${ENABLE_OPENCSG_FLAG} -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")

#
# modulecachetest
#
add_executable(modulecachetest modulecachetest.cc)
target_link_libraries(modulecachetest tests-nocgal ${GLEW_LIBRARY} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})

#
# csgtexttest
#
add_executable(csgtexttest csgtexttest.cc CSGTextRenderer.cc CSGTextCache.cc)
target_link_libraries(csgtexttest tests-nocgal ${GLEW_LIBRARY} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})

#
# cgalcachetest
#
add_executable(cgalcachetest cgalcachetest.cc)
set_target_properties(cgalcachetest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgalcachetest tests-cgal ${GLEW_LIBRARY} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})

#
# openscad no-qt
#
add_executable(openscad_nogui ../src/openscad.cc)
set_target_properties(openscad_nogui PROPERTIES COMPILE_FLAGS "-fno-strict-aliasing -DEIGEN_DONT_ALIGN ${ENABLE_OPENCSG_FLAG} -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(openscad_nogui tests-offscreen tests-cgal ${GLEW_LIBRARY} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})

# also run translation compilation to verify the files are without syntax errors
add_custom_target(locale_files ALL COMMAND ${CMAKE_SOURCE_DIR}/../scripts/translation-make.sh)

#
# GUI binary tests
#
#if(APPLE)
#  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../OpenSCAD.app/Contents/MacOS/OpenSCAD")
#elseif (MINGW_CROSS_ENV_DIR) 
#  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../mingw32/release/openscad.exe")
#elseif(WIN32)
#  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../Release/openscad.exe")
#else()
#  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_SOURCE_DIR}/../openscad")
#endif()

#if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/openscad")
#  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/openscad")
#endif()

#if(EXISTS "${OPENSCAD_BINPATH}")
#  message(STATUS "Found OpenSCAD binary: ${OPENSCAD_BINPATH}")
#else()
#  message(STATUS "Couldn't find the OpenSCAD binary: ${OPENSCAD_BINPATH}")
#  message(FATAL_ERROR "Please build the OpenSCAD binary and place it here: ${OPENSCAD_BINPATH}" )
#endif()

if(WIN32)
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/openscad_nogui.exe")
else()
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/openscad_nogui")
endif()

#
# Tags tests as disabled. This is more convenient than removing them manually
# from the lists of filenames
#
macro(disable_tests)
  foreach (TESTNAME ${ARGN})
#    message("Disabling ${TESTNAME}")
    list(APPEND DISABLED_TESTS ${TESTNAME})
  endforeach()
endmacro()

#
# Tags the given tests as belonging to the given CONFIG, i.e. will
# only be executed when run using ctest -C <CONFIG>
#
# Usage example: set_test_config(Heavy dumptest_testname opencsgtest_testname2)
#
function(set_test_config CONFIG)
  list(APPEND ${CONFIG}_TEST_CONFIG ${ARGN})
  list(FIND TEST_CONFIGS ${CONFIG} FOUND)
  if (FOUND EQUAL -1)
    list(APPEND TEST_CONFIGS ${CONFIG})
    # Export to parent scope
    set(TEST_CONFIGS ${TEST_CONFIGS} PARENT_SCOPE)
  endif()
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} PARENT_SCOPE)
endfunction()

#
# Returns a list of test configs 
#
function(get_test_config TESTNAME CONFIGS)
  foreach(CONFIG ${TEST_CONFIGS})
    list(FIND ${CONFIG}_TEST_CONFIG ${TESTNAME} IDX)
    if (${IDX} GREATER -1)
      list(APPEND ${CONFIGS} ${CONFIG})
    endif()
  endforeach()
  if (${CONFIGS})
    # Convert to a format understood by add_test()
    string(REPLACE ";" "|" ${${CONFIGS}} ${CONFIGS})
    # Export to parent scope
    set(${CONFIGS} ${${CONFIGS}} PARENT_SCOPE)
  endif()
endfunction()

#
# Returns into the FULLNAME variable the global full test name (identifier) 
# given a test command and source filename
#
function(get_test_fullname TESTCMD FILENAME FULLNAME)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  get_filename_component(TESTNAME ${FILENAME} NAME_WE)
  string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
  set(${FULLNAME} ${TESTCMD_NAME}_${TESTNAME})
  # Export to parent scope
  set(${FULLNAME} ${${FULLNAME}} PARENT_SCOPE)
endfunction()

#
# Check if a test file is a 2D test
#
function(is_2d FULLNAME RESULT)
  list(FIND ALL_2D_FILES ${FULLNAME} IDX)
  if (${IDX} GREATER -1)
    set(${RESULT} 1 PARENT_SCOPE)
  else()
    set(${RESULT} PARENT_SCOPE)
  endif()
endfunction()

#
# This functions adds cmd-line tests given files.
#
# Usage add_cmdline_test(testbasename [EXE <executable>] [ARGS <args to exe>]
#                        [SCRIPT <script>]
#                        [EXPECTEDDIR <shared dir>] SUFFIX <suffix> FILES <test files>)
#
find_package(PythonInterp)
function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "EXE;SCRIPT;SUFFIX;EXPECTEDDIR" "FILES;ARGS" ${ARGN})

  # If sharing results with another test, pass on this to the python script
  if (TESTCMD_EXPECTEDDIR)
    set(EXTRA_OPTIONS -e ${TESTCMD_EXPECTEDDIR})
  endif()

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CMAKE_BINARY_DIR}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")
    list(FIND DISABLED_TESTS ${TEST_FULLNAME} DISABLED)

    if (${DISABLED} EQUAL -1)

      set(EXPERIMENTAL_OPTION "")
      string(REGEX MATCH "csgtexttest" match_result1 ${TEST_FULLNAME})
      string(REGEX MATCH "cgalstlsanity" match_result2 ${TEST_FULLNAME})
      string(REGEX MATCH "dumptest" match_result3 ${TEST_FULLNAME})
      if( "${match_result1}" STREQUAL "" )
       if( "${match_result2}" STREQUAL "" )
        if( "${match_result3}" STREQUAL "" )
         set(EXPERIMENTAL_OPTION "")
        endif()
       endif()
      endif()

      # 2D tests should be viewed from the top, not an angle.
      set(CAMERA_OPTION "")
      is_2d(${SCADFILE} IS2D)
      if (IS2D)
        set(CAMERA_OPTION "--camera=0,0,100,0,0,0" "--viewall" "--autocenter" "--projection=ortho")
      endif()

      # Handle configurations
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      if (NOT FOUNDCONFIGS)
        set_test_config(Default ${TEST_FULLNAME})
      endif()
      set_test_config(All ${TEST_FULLNAME})
      list(FIND FOUNDCONFIGS Bugs FOUND)
      if (FOUND EQUAL -1)
        set_test_config(Good ${TEST_FULLNAME})
      endif()

      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      # The python script cannot extract the testname when given extra parameters
      if (TESTCMD_ARGS)
        set(FILENAME_OPTION -f ${FILE_BASENAME})
      endif()

      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${tests_SOURCE_DIR}/test_cmdline_tool.py --comparator=${COMPARATOR} -c ${IMAGE_COMPARE_EXECUTABLE} -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION} ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION} ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS})
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
    endif()
  endforeach()
endfunction()

#
# Usage add_failing_test(testbasename RETVAL <expected return value>
#                        [EXE <executable>] [SCRIPT <script>] [ARGS <args to exe>]
#                        FILES <test files>)
#
function(add_failing_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "RETVAL;EXE;SCRIPT;" "FILES;ARGS" ${ARGN})

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CMAKE_BINARY_DIR}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")
    list(FIND DISABLED_TESTS ${TEST_FULLNAME} DISABLED)

    if (${DISABLED} EQUAL -1)
      # Handle configurations
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      if (NOT FOUNDCONFIGS)
        set_test_config(Default ${TEST_FULLNAME})
      endif()
      set_test_config(All ${TEST_FULLNAME})
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      # The python script cannot extract the testname when given extra parameters
      if (TESTCMD_ARGS)
        set(FILENAME_OPTION -f ${FILE_BASENAME})
      endif()

      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${TESTCMD_ARGS})
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
    endif()
  endforeach()
endfunction()

enable_testing()



set_directory_properties(PROPERTIES TEST_INCLUDE_FILE "${CMAKE_SOURCE_DIR}/EnforceConfig.cmake")

# Subst files
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/include-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad)
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/use-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad)
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/import_stl-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/import_stl-tests.scad)
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/import_dxf-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/2D/features/import_dxf-tests.scad)

# Find all scad files
file(GLOB FEATURES_3D_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/*.scad)
file(GLOB FEATURES_2D_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/2D/features/*.scad)
file(GLOB DEPRECATED_3D_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/deprecated/*.scad)
file(GLOB ISSUES_3D_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/issues/*.scad)
file(GLOB SCAD_DXF_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/dxf/*.scad)
file(GLOB FUNCTION_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/functions/*.scad)
file(GLOB_RECURSE EXAMPLE_3D_FILES ${CMAKE_SOURCE_DIR}/../examples/*.scad)

list(REMOVE_ITEM EXAMPLE_3D_FILES ${CMAKE_SOURCE_DIR}/../examples/Shapes/flat_body.scad)
list(APPEND EXAMPLE_2D_FILES ${CMAKE_SOURCE_DIR}/../examples/Shapes/flat_body.scad)

list(APPEND EXAMPLE_FILES ${EXAMPLE_3D_FILES} ${EXAMPLE_2D_FILES})

list(APPEND ECHO_FILES ${FUNCTION_FILES}
            ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/for-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/expression-evaluation-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/echo-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/parser-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/builtin-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/dim-all.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-indexing.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-unicode.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/chr-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/vector-values.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/search-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/search-tests-unicode.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/recursion-test-function.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/recursion-test-function2.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/recursion-test-module.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/tail-recursion-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/value-reassignment-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/value-reassignment-tests2.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/variable-scope-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/scope-assignment-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/lookup-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/expression-shortcircuit-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/parent_module-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/children-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/range-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/no-break-space-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/unicode-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/utf8-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/nbsp-utf8-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/nbsp-latin1-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/concat-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-recursive-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/operators-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/empty-stl.scad)

list(APPEND DUMPTEST_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES})
list(APPEND DUMPTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/localfiles-test.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/localfiles_dir/localfiles-compatibility-test.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allfunctions.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allmodules.scad)

list(APPEND CGALPNGTEST_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${EXAMPLE_2D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES}
 ${EXAMPLE_3D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/transform-nan-inf-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/localfiles-test.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/localfiles_dir/localfiles-compatibility-test.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/rotate-empty-bbox.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/empty-shape-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/null-polygons.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/internal-cavity.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/internal-cavity-polyhedron.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/bad-stl-pcbvicebar.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/bad-stl-tardis.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/bad-stl-wing.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/rotate_extrude-hole.scad)

list(APPEND CGALPNGTEST_FILES ${CGALPNGTEST_2D_FILES} ${CGALPNGTEST_3D_FILES})
list(APPEND OPENCSGTEST_FILES ${CGALPNGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/intersection-prune-test.scad)
list(APPEND THROWNTOGETHERTEST_FILES ${OPENCSGTEST_FILES})

list(APPEND CGALSTLSANITYTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/normal-nan.scad)

# Issue #899
set_test_config(Bugs dxfpngtest_text-font-direction-tests
                     cgalpngtest_text-font-direction-tests
                     opencsgtest_text-font-direction-tests
                     csgpngtest_text-font-direction-tests
                     throwntogethertest_text-font-direction-tests)

list(APPEND EXPORT3D_TEST_FILES ${EXAMPLE_3D_FILES})
list(APPEND EXPORT3D_TEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/polyhedron-nonplanar-tests.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/polyhedron-tests.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/rotate_extrude-tests.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/union-coincident-test.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/mirror-tests.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/null-polygons.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/internal-cavity.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/internal-cavity-polyhedron.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/bad-stl-pcbvicebar.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/bad-stl-tardis.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/bad-stl-wing.scad
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/rotate_extrude-hole.scad)

list(APPEND EXPORTCSG_TEST_FILES 
                                ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/nonmanifold-polyhedron.scad)

disable_tests(
  # These don't output anything
  dxfpngtest_text-empty-tests
  dxfpngtest_nothing-decimal-comma-separated
  dxfpngtest_nullspace-2d

  # Not useful
  throwntogethertest_internal-cavity
  throwntogethertest_internal-cavity-polyhedron
  throwntogethertest_nullspace-difference

  # these take too long, for little relative gain in testing
  stlpngtest_iteration
  offpngtest_iteration
  stlpngtest_fractal
  offpngtest_fractal
  stlpngtest_logo_and_text
  offpngtest_logo_and_text
)

# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES ${FILES_2D})

# FIXME: This test illustrates a weakness in child() combined with modifiers.
# Reenable it when this is improved
disable_tests(opencsgtest_child-background)

# These tests only makes sense in OpenCSG mode
disable_tests(cgalpngtest_child-background
              cgalpngtest_highlight-and-background-modifier
              cgalpngtest_testcolornames
              csgpngtest_child-background
              csgpngtest_highlight-and-background-modifier
              csgpngtest_testcolornames
              throwntogethertest_testcolornames)

# This test won't render anything meaningful in throwntogether mode
disable_tests(throwntogethertest_minkowski3-erosion)

# The inf/nan tests fail when exporting CSG and rendering that output again
# as currently inf/nan is written directly to the CSG file (e.g. r = inf)
# which is not valid or even misleading in case a variable inf exists.
# FIXME: define export behavior for inf/nan when exporting CSG files
disable_tests(csgpngtest_primitive-inf-tests csgpngtest_transform-nan-inf-tests)

# Test config handling

# Heavy tests are tests taking more than 10 seconds on a development computer
set_test_config(Heavy cgalpngtest_rotate_extrude-tests
                      csgpngtest_rotate_extrude-tests
                      cgalpngtest_for-nested-tests
                      csgpngtest_for-nested-tests
                      cgalpngtest_resize-tests
                      cgalpngtest_text-search-test
                      cgalpngtest_fractal
                      csgpngtest_fractal
                      cgalpngtest_iteration
                      csgpngtest_iteration
                      cgalpngtest_linear_extrude-scale-zero-tests
                      csgpngtest_linear_extrude-scale-zero-tests
                      cgalpngtest_sphere-tests
                      csgpngtest_resize-tests
                      csgpngtest_resize-tests
                      stlpngtest_fence
                      stlpngtest_surface
                      stlpngtest_demo_cut
                      stlpngtest_search
                      stlpngtest_rounded_box
                      stlpngtest_difference
                      stlpngtest_translation
                      offpngtest_fence
                      offpngtest_surface
                      offpngtest_demo_cut
                      offpngtest_search
                      offpngtest_rounded_box
                      offpngtest_difference
                      offpngtest_translation
)

# Bugs

list(APPEND BUGS_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue13.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue13b.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue584.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue591.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue666.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue791.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue802.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue899.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue904.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue945.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue945b.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue945c.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue945d.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue1105.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue1105b.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue1105c.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue1105d.scad)
list(APPEND EXPORT3D_TEST_FILES ${BUGS_FILES})
list(REMOVE_ITEM EXPORT3D_TEST_FILES
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue899.scad
                       ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue1089.scad)
#list(APPEND EXPORTCSG_TEST_FILES )
list(APPEND ALL_2D_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/issue899.scad)

list(APPEND OPENCSGTEST_FILES ${BUGS_FILES})
list(APPEND CGALPNGTEST_FILES ${BUGS_FILES})
foreach(FILE ${BUGS_FILES})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(csgpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(offpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(monotonepngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(stlpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(stlcsgpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
endforeach()

# Examples

foreach(FILE ${EXAMPLE_FILES})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(throwntogethertest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(csgpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(monotonepngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(stlpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(offpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
endforeach()

# Workaround Gallium bugs
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
  message(STATUS "Workaround PPC bug https://bugs.freedesktop.org/show_bug.cgi?id=42540")
  set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};GALLIUM_DRIVER=softpipe")
  set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};DRAW_USE_LLVM=no")
endif()

# Set up custom commands to run before & after Ctest run.
# 1. Start/stop Virtual Framebuffer for linux/bsd. 2. Pretty Print
# Please see the CTestCustom.template file for more info. 

#
# Post-test pretty print
#

add_executable(test_pretty_print test_pretty_print.cc)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test_pretty_print.py PYSRC)
set_target_properties(test_pretty_print PROPERTIES COMPILE_FLAGS
  "-DPYBIN=${PYTHON_EXECUTABLE} -DPYSRC=${PYSRC} -DBUILDDIR=--builddir=${CMAKE_CURRENT_BINARY_DIR}"
)
#if (MINGW_CROSS_ENV_DIR)
#  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_pretty_print "test_pretty_print.exe")
#  execute_process(COMMAND chmod ugo+x ${CMAKE_CURRENT_BINARY_DIR}/test_pretty_print)
#endif()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.template TMP)
string(REPLACE __cmake_current_binary_dir__ ${CMAKE_CURRENT_BINARY_DIR} TMP ${TMP})
string(REPLACE __cmake_current_source_dir__ ${CMAKE_CURRENT_SOURCE_DIR} TMP ${TMP})
string(REPLACE __python__ ${PYTHON_EXECUTABLE} TMP ${TMP})
string(REPLACE __header__ "Generated by cmake from ${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.template" TMP ${TMP})
string(REPLACE __cmake_system_name__ ${CMAKE_SYSTEM_NAME} TMP ${TMP})
string(REPLACE __openscad_binpath__ ${OPENSCAD_BINPATH} TMP ${TMP})

set(OPENSCAD_UPLOAD_TESTS $ENV{OPENSCAD_UPLOAD_TESTS})
set(UPLOADARG "")
if (OPENSCAD_UPLOAD_TESTS)
  set(UPLOADARG "--upload")
endif()
string(REPLACE __openscad_upload_tests__ "${UPLOADARG}" TMP ${TMP})

message(STATUS "creating CTestCustom.cmake")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake ${TMP})

#
# Add tests
#
# Types of tests:
# o echotest: Just record console output
# o dumptest: Export .csg
# o cgalpngtest: Export to PNG using --render
# o opencsgtest: Export to PNG using OpenCSG
# o throwntogethertest: Export to PNG using the Throwntogether renderer
# o csgpngtest: 1) Export to .csg, 2) import .csg and export to PNG (--render)
# o monotonepngtest: Same as cgalpngtest but with the "Monotone" color scheme
# o stlpngtest: Export to STL, Re-import and render to PNG (--render=cgal)
# o offpngtest: Export to OFF, Re-import and render to PNG (--render=cgal)
# o dxfpngtest: Export to DXF, Re-import and render to PNG (--render=cgal)
# o stlcsgpngtest: Export to STL, Re-import and render to PNG (--render)
#

add_cmdline_test(moduledumptest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX ast FILES
                 ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allmodules.scad
                 ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allfunctions.scad
                 ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad)
add_cmdline_test(csgtexttest SUFFIX txt FILES
                             ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad
                             ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allfunctions.scad
                             ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allmodules.scad)
add_cmdline_test(csgtermtest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX term FILES
                             ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad
                             ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allfunctions.scad
                             ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/allmodules.scad)
add_cmdline_test(echotest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX echo FILES ${ECHO_FILES})
add_cmdline_test(dumptest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX csg FILES ${DUMPTEST_FILES})
add_cmdline_test(dumptest-examples EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX csg FILES ${EXAMPLE_FILES})
add_cmdline_test(cgalpngtest EXE ${OPENSCAD_BINPATH} ARGS --render -o SUFFIX png FILES ${CGALPNGTEST_FILES})
add_cmdline_test(opencsgtest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX png FILES ${OPENCSGTEST_FILES})
add_cmdline_test(csgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=csg --render EXPECTEDDIR cgalpngtest SUFFIX png FILES ${CGALPNGTEST_FILES})
add_cmdline_test(throwntogethertest EXE ${OPENSCAD_BINPATH} ARGS --preview=throwntogether -o SUFFIX png FILES ${THROWNTOGETHERTEST_FILES})
# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(cgalstlsanitytest EXE ${CMAKE_SOURCE_DIR}/cgalstlsanitytest SUFFIX txt ARGS ${OPENSCAD_BINPATH} FILES ${CGALSTLSANITYTEST_FILES})

#
# Export/Import tests
#

# Issue #337
set_test_config(Bugs stlpngtest_polyhedron-tests)
# Issue #910
set_test_config(Bugs offpngtest_polyhedron-tests
                     offpngtest_bad-stl-pcbvicebar
                     offpngtest_bad-stl-tardis
                     offpngtest_bad-stl-wing)
# No issue - this was introduced when fixing #1033
set_test_config(Bugs stlpngtest_bad-stl-wing)

add_cmdline_test(monotonepngtest EXE ${OPENSCAD_BINPATH} ARGS --colorscheme=Monotone --render -o SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(stlpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(offpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(dxfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D})
add_cmdline_test(stlcsgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORTCSG_TEST_FILES})


#
# Failing tests
#
add_failing_test(stlfailedtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX stl FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/empty-union.scad)
add_failing_test(offfailedtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_SOURCE_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX off FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/empty-union.scad)

#
# Add experimental tests
#

# Tests using the actual OpenSCAD binary

# non-ASCII filenames
add_cmdline_test(openscad-nonascii EXE ${OPENSCAD_BINPATH} ARGS -o 
                 SUFFIX csg 
                 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/sfære.scad)


# Variable override (-D arg)

# FIXME - this breaks on older cmake that is very common 'in the wild' on linux
# Override simple variable
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.10)
add_cmdline_test(openscad-override EXE ${OPENSCAD_BINPATH}
                 ARGS -D a=3$<SEMICOLON> -o
                 SUFFIX echo
                 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/override.scad)
endif()

# Image output parameters
add_cmdline_test(openscad-imgsize EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 100,100 -o 
                 SUFFIX png 
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
add_cmdline_test(openscad-imgstretch EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 500,100 -o 
                 SUFFIX png 
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
add_cmdline_test(openscad-imgstretch2 EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 100,500 -o 
                 SUFFIX png 
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camdist EXE ${OPENSCAD_BINPATH} 
                 ARGS --imgsize=500,500 --camera=0,0,0,90,0,90,100 -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camrot EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,0,0,440,337.5,315,100 -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camtrans EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,100 -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam, viewall
add_cmdline_test(openscad-camtrans-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,3000 --viewall -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-20,90,0,90,90 --projection=o -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Orthographic gimbal cam viewall
add_cmdline_test(openscad-camortho-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-cameye EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=60,40,30,0,0,0 -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-cameye2 EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=160,140,130,0,0,0 -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-camcenter EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=60,40,30,20,10,30  -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Perspective vector cam viewall
add_cmdline_test(openscad-camcenter-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=60,40,30,20,10,30 --viewall -o
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=160,140,130,0,0,0 --projection=o -o 
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)
# Orthographic vector cam viewall
add_cmdline_test(openscad-cameyeortho-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --projection=o -o 
                 SUFFIX png
		 FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/3D/features/camera-tests.scad)

# Colorscheme tests
add_cmdline_test(openscad-colorscheme-cornfield EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Cornfield -o 
                 SUFFIX png 
                 FILES ${CMAKE_SOURCE_DIR}/../examples/Basics/difference_cube.scad)
add_cmdline_test(openscad-colorscheme-metallic EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Metallic -o 
                 SUFFIX png 
                 FILES ${CMAKE_SOURCE_DIR}/../examples/Basics/difference_cube.scad)
add_cmdline_test(openscad-colorscheme-metallic-render EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Metallic --render -o 
                 SUFFIX png 
                 FILES ${CMAKE_SOURCE_DIR}/../examples/Basics/difference_cube.scad)
add_cmdline_test(openscad-colorscheme-sunset EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Sunset -o 
                 SUFFIX png 
                 FILES ${CMAKE_SOURCE_DIR}/../examples/Basics/difference_cube.scad)
add_cmdline_test(openscad-colorscheme-starnight EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Starnight -o 
                 SUFFIX png 
                 FILES ${CMAKE_SOURCE_DIR}/../examples/Basics/difference_cube.scad)
add_cmdline_test(openscad-colorscheme-monotone EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Monotone -o 
                 SUFFIX png 
                 FILES ${CMAKE_SOURCE_DIR}/../examples/Basics/difference_cube.scad)

#message("Available test configurations: ${TEST_CONFIGS}")
#foreach(CONF ${TEST_CONFIGS})
#  message("${CONF}: ${${CONF}_TEST_CONFIG}")
#endforeach()

message(STATUS "CPPFLAGS: ${CMAKE_CXX_FLAGS}")
