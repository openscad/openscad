# Toolchain file for cross compiling OpenSCAD linux->mingw-win32.
# 
# usage: 
#  1. get Brad Pitcher's toolchain for OpenSCAD qmake setup:
# http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#  2. export MINGW_CROSS_ENV_DIR=where-you-installed-mingw
#  2. cmake . -DCMAKE_TOOLCHAIN_FILE=CMingw-env-cross.cmake
#  3. make should proceed as normal. 
#
# see also
# 
# http://mingw-cross-env.nongnu.org/#requirements
# http://www.vtk.org/Wiki/CMake_Cross_Compiling
# https://bitbucket.org/muellni/mingw-cross-env-cmake/src/2067fcf2d52e/src/cmake-1-toolchain-file.patch
# http://code.google.com/p/qtlobby/source/browse/trunk/toolchain-mingw.cmake
# http://gcc.gnu.org/onlinedocs/gcc-3.4.6/gcc/Link-Options.html
# output of qmake 
#


#
# cross-compiler
#

set(MINGW_CROSS_ENV_DIR $ENV{MINGW_CROSS_ENV_DIR})

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_SYSTEM_NAME Windows)
set(MSYS 1)
set(CMAKE_FIND_ROOT_PATH ${MINGW_CROSS_ENV_DIR}/usr/i686-pc-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

set(CMAKE_C_COMPILER ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-g++)
set(CMAKE_RC_COMPILER ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-windres)
set(PKG_CONFIG_EXECUTABLE ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-pkg-config)
set(QT_QMAKE_EXECUTABLE ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-qmake)
set(CMAKE_INSTALL_PREFIX ${MINGW_CROSS_ENV_DIR}/usr/i686-pc-mingw32 CACHE PATH "Installation Prefix")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel")

#
# libraries
#

set( Boost_USE_STATIC_LIBS ON )
set( Boost_USE_MULTITHREADED ON )
set( Boost_COMPILER "_win32" )
# set( Boost_DEBUG TRUE ) # for debugging cmake's FindBoost, not boost itself

set( OPENSCAD_LIBRARIES ${CMAKE_FIND_ROOT_PATH} )
set( EIGEN2_DIR ${CMAKE_FIND_ROOT_PATH} )
set( CGAL_DIR ${CMAKE_FIND_ROOT_PATH}/lib/CGAL )
set( GLEW_DIR ${CMAKE_FIND_ROOT_PATH} )
set( SKIP_IMAGEMAGICK TRUE )
set( ImageMagick_convert_EXECUTABLE /usr/bin/convert )

#
# QT
#

set(QT_QMAKE_EXECUTABLE ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-qmake)
set(QT_MOC_EXECUTABLE ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-moc)
set(QT_UIC_EXECUTABLE ${MINGW_CROSS_ENV_DIR}/usr/bin/i686-pc-mingw32-uic)

#
# There are various problems introduced to cmake's linking by the large
# number of libraries needed by OpenSCAD and the complicated build system.
#
# Here we copy the flags and libraries from from looking at 
# "qmake && make VERBOSE=1" as well as examining
# the .prl files in mingw-cross-env/usr/i686-pc-mingw32/lib & using google
# Another good resource are the files under usr/i686-pc-mingw32/mkspecs
#

if (NOT MINGW_CROSS_FLAGS_SET)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UQT_DLL -frtti -fexceptions -mthreads -Wall -DQT_LARGEFILE_SUPPORT")
  set(MINGW_CROSS_FLAGS_SET 1)
endif()

function(mingw_cross_env_info)
  message(STATUS "QT USE FILE: ${QT_USE_FILE}")
  message(STATUS "QT INCLUDES: ${QT_INCLUDES}")
  message(STATUS "QT LIBRARY_DIR: ${QT_LIBRARY_DIR}")
  message(STATUS "QT LIBRARIES: ${QT_LIBRARIES}")
endfunction()

function(mingw_cross_env_add_missing_libs)
  # mingw_cross_env_info()
  # some systems may need more libs, like -lzma
  set(mingw_cross_libs imm32 winmm ws2_32 glu32 opengl32 mng lcms tiff jpeg png z)
  target_link_libraries(opencsgtest ${mingw_cross_libs})
  target_link_libraries(csgtermtest ${mingw_cross_libs})
  target_link_libraries(csgtexttest ${mingw_cross_libs})
  target_link_libraries(dumptest ${mingw_cross_libs})
  target_link_libraries(echotest ${mingw_cross_libs})
  target_link_libraries(cgalpngtest ${mingw_cross_libs})
  target_link_libraries(throwntogethertest ${mingw_cross_libs})
  target_link_libraries(cgaltest ${mingw_cross_libs})
  target_link_libraries(cgalstlsanitytest ${mingw_cross_libs})
endfunction()

#
# -D definitions
#

if( NOT CROSS_DEFS_SET )
  add_definitions( -DQT_THREAD_SUPPORT )
  add_definitions( -DGLEW_STATIC )
  add_definitions( -DBOOST_STATIC )
  add_definitions( -DBOOST_THREAD_USE_LIB )
  add_definitions( -DUNICODE )

  set(CROSS_DEFS_SET 1)

endif()



