# instructions - see ../doc/testing.txt

cmake_minimum_required(VERSION 2.8)
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.3)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()

# Detect Lion and force gcc
IF (APPLE)
   EXECUTE_PROCESS(COMMAND sw_vers -productVersion OUTPUT_VARIABLE MACOSX_VERSION)
   IF (NOT ${MACOSX_VERSION} VERSION_LESS "10.7.0")
     message("Detected Lion or later")
     set(CMAKE_C_COMPILER "gcc")
     set(CMAKE_CXX_COMPILER "g++")
   ELSE()
     message("Detected Snow Leopard or older")
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
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
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

if(WIN32 AND CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -frounding-math")
endif()

# Clang compiler

if(CMAKE_CXX_COMPILER MATCHES ".*clang.*")
  # disable enormous amount of warnings about CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-extensions")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

#
# Build test apps
#

function(inclusion user_set_path found_paths)
  # If user_set_path indicates an env. variable was specifically
  # set by the user, then found_paths become an include priority (prepend);
  # otherwise found_paths are stuck on the end of the include flags (append).

  # message(STATUS "inclusion ${user_set_path} ${found_paths}")
  # message(STATUS "inclusion ${${user_set_path}} ${${found_paths}}")
  set( inclusion_match 0 )
  foreach( found_path ${${found_paths}} )
    if (${found_path} MATCHES ${${user_set_path}}.*)
      set( inclusion_match 1 )
    endif()
  endforeach()
  if (user_set_path AND inclusion_match)
    include_directories(BEFORE ${${found_paths}})
    # message(STATUS "inclusion prepend ${${found_paths}} for ${user_set_path}")
  else()
    include_directories(AFTER ${${found_paths}})
    # message(STATUS "inclusion append ${${found_paths}} for ${user_set_path}")
  endif()
  set( inclusion_match 0 )
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
message(STATUS "Boost includes found: " ${Boost_INCLUDE_DIRS})
message(STATUS "Boost libraries found:")
foreach(boostlib ${Boost_LIBRARIES})
  message(STATUS "  " ${boostlib})
endforeach()

inclusion(BOOST_ROOT Boost_INCLUDE_DIRS)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3) # Use V3 for boost 1.44-1.45

# Mac OS X
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  FIND_LIBRARY(COCOA_LIBRARY Cocoa REQUIRED)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Qt4

set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
find_package(OpenGL REQUIRED)

if (MINGW_CROSS_ENV_DIR) 
  mingw_cross_env_find_qt()
  mingw_cross_env_info()
  include_directories( ${QT_INCLUDE_DIRS} )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QT_CFLAGS_OTHER}")
else()
  find_package(Qt4 COMPONENTS QtCore REQUIRED)
  include(${QT_USE_FILE})
endif()

set(CMAKE_INCLUDE_DIRECTORIES_BEFORE OFF)


# Eigen

# Turn off Eigen SIMD optimization
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEIGEN_DONT_ALIGN")
  endif()
endif()

# Priority
# 3. EIGENDIR if set (EIGEN2DIR for backwards compatability)
# 1. OPENSCAD_LIBRARIES eigen3
# 2. OPENSCAD_LIBRARIES eigen2
# 4. system's standard include paths for eigen3
# 5. system's standard include paths for eigen2

set(EIGEN2_DIR "$ENV{EIGEN2DIR}")
set(EIGEN_DIR "$ENV{EIGENDIR}")
set(OPENSCAD_LIBDIR "$ENV{OPENSCAD_LIBRARIES}")

if (EIGEN_DIR)
  set(EIGHINT ${EIGEN_DIR}/include/eigen3 ${EIGEN_DIR}/include/eigen2 ${EIGEN_DIR})
  find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS ${EIGHINT})
endif()
if (EIGEN2_DIR)
  find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS ${EIGEN2_DIR}/include/eigen2 ${EIGEN2_DIR})
endif()

if (NOT EIGEN_INCLUDE_DIR)
  find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS ${OPENSCAD_LIBDIR}/include/eigen3)
endif()
if (NOT EIGEN_INCLUDE_DIR)
  find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS ${OPENSCAD_LIBDIR}/include/eigen2)
endif()

if (NOT EIGEN_INCLUDE_DIR)
  if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/local/include/eigen3)
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/pkg/include/eigen3)
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /opt/local/include/eigen3)
  else()
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/include/eigen3)
  endif()
endif()

if (NOT EIGEN_INCLUDE_DIR)
  if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/local/include/eigen2)
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/pkg/include/eigen2)
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /opt/local/include/eigen2)
  else()
    find_path(EIGEN_INCLUDE_DIR Eigen/Core HINTS /usr/include/eigen2)
  endif()
endif()

if (NOT EIGEN_INCLUDE_DIR)
  message(STATUS "Eigen not found")
else()
  message(STATUS "Eigen found in " ${EIGEN_INCLUDE_DIR})
  inclusion(EIGEN_DIR EIGEN_INCLUDE_DIR)
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
if (NOT GLEW_INCLUDE_DIR)
  message(STATUS "OPENCSG_DIR: " ${OPENCSG_DIR})
  find_path(GLEW_INCLUDE_DIR
            GL/glew.h
            HINTS ${GLEW_DIR}/include
	    NO_DEFAULT_PATH)
  find_library(GLEW_LIBRARY
               NAMES GLEW glew
               HINTS ${GLEW_DIR}/lib ${GLEW_DIR}/lib64
	       NO_DEFAULT_PATH)
  if (NOT GLEW_LIBRARY)
    find_package(GLEW REQUIRED)
    if (NOT GLEW_LIBRARY)
      message(FATAL_ERROR "GLEW not found")
    endif()
  endif()
  message(STATUS "GLEW include: " ${GLEW_INCLUDE_DIR})
  message(STATUS "GLEW library: " ${GLEW_LIBRARY})
endif()

inclusion(GLEW_DIR GLEW_INCLUDE_DIR)

# Flex/Bison
find_package(BISON REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  # FreeBSD has an old flex in /usr/bin and a new flex in /usr/local/bin
  set(FLEX_EXECUTABLE /usr/local/bin/flex)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
  include_directories( /usr/pkg/include /usr/X11R7/include )
  set(FLEX_EXECUTABLE /usr/pkg/bin/flex)
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

if (NOT $ENV{CGALDIR} STREQUAL "")
  set(CGAL_DIR "$ENV{CGALDIR}")
elseif (NOT $ENV{OPENSCAD_LIBRARIES} STREQUAL "")
  if (EXISTS "$ENV{OPENSCAD_LIBRARIES}/lib/CGAL")
    set(CGAL_DIR "$ENV{OPENSCAD_LIBRARIES}/lib/CGAL")
    set(CMAKE_MODULE_PATH "${CGAL_DIR}")
  elseif (EXISTS "$ENV{OPENSCAD_LIBRARIES}/include/CGAL")
    set(CGAL_DIR "$ENV{OPENSCAD_LIBRARIES}")
    set(CMAKE_MODULE_PATH "${CGAL_DIR}")
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

# Imagemagick

if (NOT SKIP_IMAGEMAGICK)
  find_package(ImageMagick COMPONENTS convert)
  if (ImageMagick_convert_FOUND)
    message(STATUS "ImageMagick convert executable found: " ${ImageMagick_convert_EXECUTABLE})
  else()
    message(FATAL_ERROR "Couldn't find imagemagick 'convert' program")
  endif()
endif()

# Internal includes
include_directories(../src)

add_definitions(-DOPENSCAD_VERSION=test -DOPENSCAD_YEAR=2011 -DOPENSCAD_MONTH=10)
add_definitions(-DOPENSCAD_TESTING)

set(CORE_SOURCES
  tests-common.cc 
  ../src/parsersettings.cc
  ../src/mathc99.cc
  ../src/linalg.cc
  ../src/handle_dep.cc 
  ../src/value.cc 
  ../src/expr.cc 
  ../src/func.cc 
  ../src/module.cc 
  ../src/ModuleCache.cc 
  ../src/node.cc 
  ../src/context.cc 
  ../src/csgterm.cc 
  ../src/csgtermnormalizer.cc 
  ../src/polyset.cc 
  ../src/csgops.cc 
  ../src/transform.cc 
  ../src/color.cc 
  ../src/primitives.cc 
  ../src/projection.cc 
  ../src/cgaladv.cc 
  ../src/surface.cc 
  ../src/control.cc 
  ../src/render.cc 
  ../src/dxfdata.cc 
  ../src/dxfdim.cc 
  ../src/linearextrude.cc 
  ../src/rotateextrude.cc 
  ../src/printutils.cc 
  ../src/progress.cc 
  ${FLEX_OpenSCADlexer_OUTPUTS}
  ${BISON_OpenSCADparser_OUTPUTS})

set(NOCGAL_SOURCES
  ../src/builtin.cc 
  ../src/dxftess.cc 
  ../src/import.cc
  ../src/export.cc) 

set(CGAL_SOURCES
  ${NOCGAL_SOURCES}
	  ../src/CSGTermEvaluator.cc 
  ../src/CGAL_Nef_polyhedron.cc 
  ../src/cgalutils.cc 
  ../src/CGALEvaluator.cc
  ../src/CGALCache.cc
  ../src/PolySetCGALEvaluator.cc
  ../src/CGAL_Nef_polyhedron_DxfData.cc
  ../src/cgaladv_minkowski2.cc)

set(COMMON_SOURCES
  ../src/nodedumper.cc 
  ../src/traverser.cc 
  ../src/PolySetEvaluator.cc 
  ../src/PolySetCache.cc 
  ../src/Tree.cc
  lodepng.cpp)

#
# Offscreen OpenGL context source code
#
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	message(STATUS "Offscreen OpenGL Context - using Apple CGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContext.mm" CACHE TYPE STRING)
elseif(UNIX)
	message(STATUS "Offscreen OpenGL Context - using Unix GLX")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextGLX.cc" CACHE TYPE STRING)
elseif(WIN32)
	message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextWGL.cc" CACHE TYPE STRING)
endif()

set(OFFSCREEN_SOURCES
  OffscreenView.cc
  ${OFFSCREEN_CTX_SOURCE}
  imageutils.cc
  fbo.cc
  system-gl.cc)

add_library(tests-core STATIC ${CORE_SOURCES})
target_link_libraries(tests-core ${OPENGL_LIBRARY})
set(TESTS-CORE-LIBRARIES ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

add_library(tests-common STATIC ${COMMON_SOURCES})
target_link_libraries(tests-common tests-core)

add_library(tests-cgal STATIC ${CGAL_SOURCES})
set_target_properties(tests-cgal PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(tests-cgal tests-common)
set(TESTS-CGAL-LIBRARIES ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${GMP_LIBRARIES} ${MPFR_LIBRARIES} ${TESTS-CORE-LIBRARIES})

add_library(tests-nocgal STATIC ${NOCGAL_SOURCES})
target_link_libraries(tests-nocgal tests-common)
add_library(tests-offscreen STATIC ${OFFSCREEN_SOURCES})
# set_target_properties(tests-offscreen PROPERTIES COMPILE_FLAGS "-DENABLE_OPENCSG -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
set(TESTS-NOCGAL-LIBRARIES ${TESTS-CORE-LIBRARIES})

#
# echotest
#
add_executable(echotest echotest.cc)
target_link_libraries(echotest tests-nocgal ${TESTS-NOCGAL-LIBRARIES})

#
# dumptest
#
add_executable(dumptest dumptest.cc)
target_link_libraries(dumptest tests-nocgal ${TESTS-NOCGAL-LIBRARIES})

#
# modulecachetest
#
add_executable(modulecachetest modulecachetest.cc)
target_link_libraries(modulecachetest tests-nocgal ${TESTS-NOCGAL-LIBRARIES})

#
# csgtexttest
#
add_executable(csgtexttest csgtexttest.cc CSGTextRenderer.cc CSGTextCache.cc)
target_link_libraries(csgtexttest tests-nocgal ${TESTS-NOCGAL-LIBRARIES})

#
# csgtermtest
#
add_executable(csgtermtest csgtermtest.cc ../src/CSGTermEvaluator.cc)
target_link_libraries(csgtermtest tests-nocgal ${TESTS-NOCGAL-LIBRARIES})

#
# cgaltest
#
add_executable(cgaltest cgaltest.cc)
set_target_properties(cgaltest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgaltest tests-cgal ${TESTS-CGAL-LIBRARIES})

#
# cgalstlsanitytest
#
add_executable(cgalstlsanitytest cgalstlsanitytest.cc)
set_target_properties(cgalstlsanitytest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgalstlsanitytest tests-cgal ${TESTS-CGAL-LIBRARIES})

#
# cgalpngtest
#
add_executable(cgalpngtest cgalpngtest.cc bboxhelp.cc ../src/CGALRenderer.cc ../src/renderer.cc ../src/rendersettings.cc)
set_target_properties(cgalpngtest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgalpngtest tests-offscreen tests-cgal ${OPENCSG_LIBRARY} ${TESTS-CGAL-LIBRARIES} ${GLEW_LIBRARY} ${COCOA_LIBRARY})

#
# cgalcachetest
#
add_executable(cgalcachetest cgalcachetest.cc bboxhelp.cc)
set_target_properties(cgalcachetest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgalcachetest tests-cgal ${TESTS-CGAL-LIBRARIES} ${GLEW_LIBRARY} ${COCOA_LIBRARY})

#
# opencsgtest
#

add_executable(opencsgtest opencsgtest.cc csgtestcore.cc ../src/OpenCSGRenderer.cc ../src/ThrownTogetherRenderer.cc ../src/renderer.cc ../src/rendersettings.cc)
set_target_properties(opencsgtest PROPERTIES COMPILE_FLAGS "-DENABLE_OPENCSG -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(opencsgtest tests-offscreen tests-cgal ${OPENCSG_LIBRARY} ${TESTS-CGAL-LIBRARIES} ${GLEW_LIBRARY} ${COCOA_LIBRARY})

#
# throwntogethertest
#

add_executable(throwntogethertest throwntogethertest.cc csgtestcore.cc ../src/OpenCSGRenderer.cc ../src/ThrownTogetherRenderer.cc ../src/renderer.cc ../src/rendersettings.cc)
set_target_properties(throwntogethertest PROPERTIES COMPILE_FLAGS "-DENABLE_OPENCSG -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(throwntogethertest tests-offscreen tests-cgal ${OPENCSG_LIBRARY} ${TESTS-CGAL-LIBRARIES} ${GLEW_LIBRARY} ${COCOA_LIBRARY})

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

# comparison method to use
if (NOT $ENV{COMPARATOR} STREQUAL "")
  set(COMPARATOR "$ENV{COMPARATOR}")
  message(STATUS "ImageMagick method modified with COMPARATOR: " ${COMPARATOR})
endif()

#
# This functions adds cmd-line tests given files.
# Files are sent as the parameters following TESTSUFFIX
#
# Usage add_cmdline_test(TESTCMD TESTSUFFIX <test files>)
#
find_package(PythonInterp)
macro(add_cmdline_test TESTCMD TESTSUFFIX)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)

  # Add tests from args
  foreach (SCADFILE ${ARGN})
    get_filename_component(TESTNAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_NAME}_${TESTNAME}")
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

      if (MINGW_CROSS_ENV_DIR)
        set( MINGW_CROSS_ARG "--mingw-cross-env" )
      endif()
      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${tests_SOURCE_DIR}/test_cmdline_tool.py ${MINGW_CROSS_ARG} --comparator=${COMPARATOR} -c ${ImageMagick_convert_EXECUTABLE} -s ${TESTSUFFIX} ${CMAKE_BINARY_DIR}/${TESTCMD} "${SCADFILE}")
    endif()
  endforeach()
endmacro()

enable_testing()

# set up custom pretty printing of results

set(INFOCMD "execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/opencsgtest --info OUTPUT_FILE sysinfo.txt)")
if (MINGW_CROSS_ENV_DIR)
  set(INFOCMD "execute_process(COMMAND wine ${CMAKE_CURRENT_BINARY_DIR}/opencsgtest --info OUTPUT_FILE sysinfo.txt)")
endif()
set(PRETTYCMD "\"${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_pretty_print.py --builddir=${CMAKE_CURRENT_BINARY_DIR}\"")
set(CTEST_CUSTOM_FILE ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake)
set(CTEST_CUSTOM_TXT "\n
  message(\"running 'opencsgtest --info' to generate sysinfo.txt\")\n
  ${INFOCMD}\n
  set(CTEST_CUSTOM_POST_TEST ${PRETTYCMD})\n
")
file(WRITE ${CTEST_CUSTOM_FILE} ${CTEST_CUSTOM_TXT})

foreach(FILE test_pretty_print.py)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
                 ${CMAKE_CURRENT_BINARY_DIR}/${FILE} COPYONLY)
endforeach()

set_directory_properties(PROPERTIES TEST_INCLUDE_FILE "${CMAKE_SOURCE_DIR}/EnforceConfig.cmake")

# Subst files
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/include-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad)
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/use-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad)
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/import_stl-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/features/import_stl-tests.scad)
configure_file(${CMAKE_SOURCE_DIR}/../testdata/scad/templates/import_dxf-tests-template.scad
               ${CMAKE_SOURCE_DIR}/../testdata/scad/features/import_dxf-tests.scad)

# Find all scad files
file(GLOB MINIMAL_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/minimal/*.scad)
file(GLOB FEATURES_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/features/*.scad)
file(GLOB BUGS_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/*.scad)
file(GLOB SCAD_DXF_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/dxf/*.scad)
file(GLOB FUNCTION_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/functions/*.scad)
file(GLOB EXAMPLE_FILES ${CMAKE_SOURCE_DIR}/../examples/*.scad)

list(APPEND ECHO_FILES ${FUNCTION_FILES}
            ${CMAKE_SOURCE_DIR}/../testdata/scad/minimal/echo.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/echo-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/parser-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/builtin-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/dim-all.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-indexing.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/vector-values.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/search-tests.scad)

list(APPEND DUMPTEST_FILES ${MINIMAL_FILES} ${FEATURES_FILES} ${EXAMPLE_FILES})
list(APPEND DUMPTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad)

list(APPEND CGALPNGTEST_FILES ${FEATURES_FILES} ${SCAD_DXF_FILES} ${EXAMPLE_FILES})
list(APPEND CGALPNGTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
                           ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad)
list(APPEND OPENCSGTEST_FILES ${CGALPNGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/bbox-transform-bug.scad)
list(APPEND OPENCSGTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/intersection-prune-test.scad)
list(APPEND THROWNTOGETHERTEST_FILES ${OPENCSGTEST_FILES})

list(APPEND CGALSTLSANITYTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/normal-nan.scad)

# Disable tests which are known to cause floating point comparison issues
# Once we're capable of comparing these across platforms, we can put these back in
disable_tests(dumptest_transform-tests
              dumptest_render-tests
              dumptest_difference-tests
              dumptest_intersection-tests
              dumptest_text-search-test
              dumptest_example001
              dumptest_example005
              dumptest_example006
              dumptest_example007
              dumptest_example008
              dumptest_example012
              dumptest_example013
              dumptest_example016
              dumptest_example017
              dumptest_example020
              dumptest_example021
              dumptest_example022
              dumptest_example023)

# FIXME: This test illustrates a weakness in child() combined with modifiers.
# Reenable it when this is improved
disable_tests(opencsgtest_child-background)

# FIXME: This single test takes over an hour to run on a 2.7 GHz P4
disable_tests(opencsgtest_example006 cgalpngtest_example006)

# These tests only makes sense in OpenCSG mode
disable_tests(cgalpngtest_child-background
              cgalpngtest_highlight-and-background-modifier
              cgalpngtest_testcolornames
              throwntogethertest_child-background
              throwntogethertest_highlight-and-background-modifier
              throwntogethertest_testcolornames)

# Test config handling

set_test_config(Heavy opencsgtest_minkowski3-tests
                      opencsgtest_projection-tests
                      throwntogethertest_minkowski3-tests
                      throwntogethertest_projection-tests
                      cgalpngtest_projection-tests
                      cgalpngtest_rotate_extrude-tests
                      cgalpngtest_surface-tests
                      cgalpngtest_sphere-tests
                      cgalpngtest_minkowski3-tests
                      cgalpngtest_for-tests
                      cgalpngtest_for-nested-tests
                      cgalpngtest_difference-tests
                      cgalpngtest_intersection-tests)

foreach(FILE ${EXAMPLE_FILES})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(throwntogethertest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
endforeach()

# Add tests

add_cmdline_test(echotest txt ${ECHO_FILES})
add_cmdline_test(dumptest txt ${DUMPTEST_FILES})
add_cmdline_test(csgtexttest txt ${MINIMAL_FILES})
add_cmdline_test(csgtermtest txt ${MINIMAL_FILES})
add_cmdline_test(cgalpngtest png ${CGALPNGTEST_FILES})
add_cmdline_test(opencsgtest png ${OPENCSGTEST_FILES})
add_cmdline_test(throwntogethertest png ${THROWNTOGETHERTEST_FILES})
# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(cgalstlsanitytest txt ${CGALSTLSANITYTEST_FILES})

message("Available test configurations: ${TEST_CONFIGS}")
#foreach(CONF ${TEST_CONFIGS})
#  message("${CONF}: ${${CONF}_TEST_CONFIG}")
#endforeach()
