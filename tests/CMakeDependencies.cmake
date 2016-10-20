# find dependency libraries and programs



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


# find the pkg-config based libraries first.

if (DEFINED ENV{OPENSCAD_LIBRARIES})
  set(ENV{PKG_CONFIG_PATH} "$ENV{OPENSCAD_LIBRARIES}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif()
message(STATUS "PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH}")

find_package(PkgConfig REQUIRED)
include(PkgConfigTools.cmake)
save_pkg_config_env()

pkg_check_modules(EIGEN REQUIRED eigen3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig>=2.8.0)
pkg_check_modules(FREETYPE REQUIRED freetype2>=2.4.9)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz>=0.9.19)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
find_package(LibXml2 2.9 REQUIRED)

add_definitions(${EIGEN_CFLAGS})
add_definitions(${GLEW_CFLAGS})
add_definitions(${FONTCONFIG_CFLAGS})
add_definitions(${FREETYPE_CFLAGS})
add_definitions(${HARFBUZZ_CFLAGS})
add_definitions(${GLIB2_DEFINITIONS})
add_definitions(${LIBXML2_DEFINITIONS})
#inclusion(GLIB2_DIR GLIB2_INCLUDE_DIRS)
#inclusion(LIBXML2_DIR LIBXML2_INCLUDE_DIR)

restore_pkg_config_env()

# Turn off Eigen SIMD optimization
if(NOT APPLE)
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "^FreeBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEIGEN_DONT_ALIGN")
  endif()
endif()


# Boost

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

find_package( Boost 1.48.0 COMPONENTS thread program_options filesystem system regex REQUIRED)
message(STATUS "Boost ${Boost_VERSION} includes found: " ${Boost_INCLUDE_DIRS})
message(STATUS "Boost libraries found:")
foreach(boostlib ${Boost_LIBRARIES})
  message(STATUS "  " ${boostlib})
endforeach()

inclusion(BOOST_ROOT Boost_INCLUDE_DIRS)

# On Mac, we need to link against the correct C++ library. We choose the same one
# as Boost uses.
if(APPLE)
  execute_process(COMMAND grep -q __112basic_string ${Boost_LIBRARIES}
                  RESULT_VARIABLE BOOST_USE_STDLIBCPP)
  if (NOT BOOST_USE_STDLIBCPP)
     message(STATUS "Found boost linked with libc++")
     set(LIBCXX11 TRUE)
  else()
     message(STATUS "Found boost linked with libstdc++")
     set(LIBCXX11 FALSE)
  endif()
endif()

# Mac OS X
if(APPLE)
  FIND_LIBRARY(COCOA_LIBRARY Cocoa REQUIRED)
  FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices)
endif()

# OpenGL
find_package(OpenGL REQUIRED)
if (NOT OPENGL_GLU_FOUND)
  message(STATUS "GLU not found in system paths...searching $ENV{OPENSCAD_LIBRARIES} ")
  find_library(OPENGL_glu_LIBRARY GLU HINTS $ENV{OPENSCAD_LIBRARIES}/lib)
  if (NOT OPENGL_glu_LIBRARY)
    message(FATAL "GLU library not found")
  endif()
  set(OPENGL_LIBRARIES ${OPENGL_glu_LIBRARY} ${OPENGL_LIBRARIES})
endif()
message(STATUS "OPENGL_LIBRARIES: ")
foreach(GLLIB ${OPENGL_LIBRARIES})
  message(STATUS "  " ${GLLIB})
endforeach()
if (UNIX)
  # see github issue 1355. as of writing the code for offscreen GL context
  # setup requires X11 on Un*x like systems (not Apple/Win).
  find_package(X11 REQUIRED)
  message(STATUS "X11_INCLUDE_DIR: " ${X11_INCLUDE_DIR})
  message(STATUS "X11_LIBRARIES: ")
  foreach(XLIB ${X11_LIBRARIES})
    message(STATUS "  " ${XLIB})
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


# prepend the dir where deps were built
if (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  set(OSCAD_DEPS "")
  set(OSCAD_DEPS_PATHS $ENV{OPENSCAD_LIBRARIES}/include)
  inclusion(OSCAD_DEPS OSCAD_DEPS_PATHS)
endif()

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

MESSAGE(STATUS "CGAL 3RD PARTY LIBS:")
foreach(CGAL3RDPLIB ${CGAL_3RD_PARTY_LIBRARIES})
  MESSAGE(STATUS " ${CGAL3RDPLIB}" )
endforeach()

# Flex/Bison
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
# The COMPILE_FLAGS and forced C++ compiler is just to be compatible with qmake
if (WIN32)
  set(FLEX_UNISTD_FLAG "-DYY_NO_UNISTD_H")
endif()
FLEX_TARGET(OpenSCADlexer ../src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Plexer ${FLEX_UNISTD_FLAG}")
BISON_TARGET(OpenSCADparser ../src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c COMPILE_FLAGS "-p parser")
ADD_FLEX_BISON_DEPENDENCY(OpenSCADlexer OpenSCADparser)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c PROPERTIES LANGUAGE "CXX")

